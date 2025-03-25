// #define _GNU_SOURCE

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "../../lib/CondVar.h"
#include "../../lib/Lock.h"
#include "../../lib/debug.cpp"
#include "../../lib/uthread.h"
#include "async_socket.h"
#include "connection_queue.h"
#include "http.h"

#define BUFSIZE 512
#define LISTEN_QUEUE_LEN 5
#define N_THREADS 5

static connection_queue_t queue;
const char *serve_dir;
int keep_going = 1;

// Thread Function
void *handle_http_request(void *arg) {
    connection_queue_t *queue = (connection_queue_t *) arg;
    char resource_name[BUFSIZE];
    // Worker thread reads and responds to http requests
    while (keep_going == 1) {
        // strcpy will overwrite the old buffer contents
        strcpy(resource_name, serve_dir);
        // Get a client file descriptor from connection queue
        PRINT("Thread %d waiting in connection queue\n", uthread_self());
        int client_fd = connection_queue_dequeue(queue);
        if (client_fd == -1) {
            continue;
        }
        // Read in the http resquest
        printf("Thread %d reading client request\n", uthread_self());
        if (read_http_request(client_fd, resource_name) == -1) {
            close(client_fd);
            continue;
        }
        // Write response
        PRINT("Thread %d writing server response\n", uthread_self());
        if (write_http_response(client_fd, resource_name) == -1) {
            close(client_fd);
            continue;
        }
        // Close the client file descriptor
        PRINT("Thread %d closing connection\n", uthread_self());
        if (close(client_fd) == -1) {
            perror("close");
        }
    }
    PRINT("Thread %d exiting\n", uthread_self());
    return NULL;
}

/**
 * Helper Function: cleans up the connection queue and joins all ptheads
 * Used to quickly clean up after an error
 * queue: connection queue
 * threads: pthread_t array
 * nthreads: length of pthread_t array
 */
void clean_up(connection_queue_t *queue, int *threads, int nthreads) {
    keep_going = 0;
    connection_queue_shutdown(queue);
    for (int i = 0; i < nthreads; i++) {
        uthread_join(threads[i], NULL);
    }
}

void handle_sigint(int signo) {
    (void) signo;
    fprintf(stderr, "SIGINT recieved\n");
    keep_going = 0;
    connection_queue_shutdown(&queue);
    // Abort if server is failing to shutdown
    static int num_sig_caught = 0;
    if (num_sig_caught++ > 3) {
        abort();
    }
}

int main(int argc, char **argv) {
    // First argument is directory to server, second is port, third is quantum
    if (argc != 4) {
        printf("Usage: %s <directory> <port> <quantum>\n", argv[0]);
        return 1;
    }

    serve_dir = argv[1];
    const char *port = argv[2];
    const int quantum_usecs = atoi(argv[3]);

    // Initialize uthread library
    if (uthread_init(quantum_usecs) == -1) {
        fprintf(stderr, "uthread_init\n");
        return 1;
    }

    // Setup signal handler to catch SIGINT
    struct sigaction sact;
    sact.sa_handler = handle_sigint;
    sact.sa_flags = 0;
    if (sigfillset(&sact.sa_mask) == -1) {
        perror("sigfillset");
        return 1;
    }
    // Install signal handler
    if (sigaction(SIGINT, &sact, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    // Setup for getaddrinfo
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *server;
    // Setup address info for socket
    int ret_val = getaddrinfo(NULL, port, &hints, &server);
    if (ret_val != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(ret_val));
        return 1;
    }

    // Initialize socket file descriptor
    int sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if (sock_fd == -1) {
        perror("socket");
        freeaddrinfo(server);
        return 1;
    }

    // Bind socket to receive at a specific port
    if (bind(sock_fd, server->ai_addr, server->ai_addrlen)) {
        perror("bind");
        freeaddrinfo(server);
        close(sock_fd);
        return 1;
    }

    // Done with server setup
    freeaddrinfo(server);

    // Designate socket as a server socket
    if (listen(sock_fd, LISTEN_QUEUE_LEN) == -1) {
        perror("listen");
        close(sock_fd);
        return 1;
    }

    // Initalize the connection queue
    if (connection_queue_init(&queue) == -1) {
        close(sock_fd);
        return 1;
    }

    // Create and initialize workers threads in thread pool
    int threads[N_THREADS];
    for (int i = 0; i < N_THREADS; i++) {
        threads[i] = uthread_create(handle_http_request, &queue);
        if (threads[i] == -1) {
            fprintf(stderr, "uthread_create\n");
            // Clean up
            keep_going = 0;
            connection_queue_shutdown(&queue);
            for (int j = 0; j < i; j++) {
                uthread_join(threads[j], NULL);
            }
            close(sock_fd);
            return 1;
        }
    }

    // Communicate with client
    while (keep_going == 1) {
        // Wait to to recieve a connection request from client
        int client_fd = async_accept(sock_fd, NULL, NULL);
        if (client_fd == -1) {
            // Check if accept failed from something other than SIGINT
            if (errno != EINTR) {
                perror("accept");
                // Cleans up queue and joins all threads
                clean_up(&queue, threads, N_THREADS);
                close(sock_fd);
                return 1;
            }
            // Otherwise break out of while loop
            break;
        }
        // Enqueue the client file descriptor to connection queue
        PRINT("Thread %d ready to enqueue client fd\n", uthread_self());
        if (connection_queue_enqueue(&queue, client_fd) == -1) {
            close(client_fd);
        }
    }

    // Clean up connection queue
    ret_val = 0;
    if (connection_queue_shutdown(&queue) == -1) {
        ret_val = 1;
    }
    // Clean up worker threads
    for (int i = 0; i < N_THREADS; i++) {
        if (uthread_join(threads[i], NULL) != 0) {
            fprintf(stderr, "uthread_join\n");
            ret_val = 1;
        }
    }
    // Close socket file descriptor
    if (close(sock_fd) == -1) {
        perror("close");
        ret_val = 1;
    }
    // Exit uthread library
    uthread_exit(NULL);
    return ret_val;
}
