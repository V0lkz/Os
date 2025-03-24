#include "async_socket.h"

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <sys/socket.h>

#include "../../lib/uthread.h"

extern int keep_going;

int async_accept(int sockfd, struct sockaddr *addr, unsigned int *addrlen) {
    // Create pollfds array
    struct pollfd pfds[1];
    pfds[0].fd = sockfd;
    pfds[0].events = POLLIN;

    // Poll sockfd and yield thread until it is ready
    int ret_val;
    while ((ret_val = poll(pfds, 1, POLL_TIMEOUT)) != 1) {
        // Check if poll returned an error
        if (ret_val == -1 && errno != EINTR) {
            perror("poll");
            return -1;
        }
        // Check if SIGINT recieved
        if (keep_going != 1) {
            return -1;
        }
        // Yield the thread
        if (uthread_yield() != 0) {
            fprintf(stderr, "uthread_yield\n");
            return -1;
        }
    }

    // Call accept and return result
    return accept(sockfd, addr, addrlen);
}
