#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <vector>

#include "../lib/async_io.h"
#include "../lib/uthread.h"

enum IOType {
    SYNC,
    ASYNC
};

struct ThreadArg {
    IOType io_type;
    int num_ops;
    int num_iters;
    size_t op_size;
};

static int filedes;
static off_t offset = 0;
volatile long x = 0;
char *buf = nullptr;

void add_workload(int num_iters) {
    for (int i = 0; i < num_iters; ++i) {
        x += i * i;
    }
}

// Thread function
void *thread_io(void *args) {
    ThreadArg *params = (ThreadArg *) args;
    IOType mode = params->io_type;
    int num_ops = params->num_ops;
    int num_iters = params->num_iters;
    int length = params->op_size;

    // Write thread id as a string
    int tid = uthread_self();
    memset(buf, (tid % 28) + 96, length);

    if (offset == 0) {
        offset -= length;
    }

    add_workload(num_iters);

    // Write loop
    for (int i = 0; i < num_ops; ++i) {
        // Write I/O to completion
        if (mode == SYNC) {
            if (write(filedes, buf, length) != length) {
                perror("write");
            }
            offset += length;
        }
        // Allow other threads to run while waiting for I/O
        else {
            // Reserve space in file for id
            offset += length;
            if (async_write(filedes, buf, length, offset - length) != length) {
                perror("async_write");
            }
        }
    }
    return nullptr;
}

// Run thread test
double run_test(int num_threads, int num_ops, IOType mode, size_t op_size, int num_iter) {
    // Create thread array and args
    int *tids = (int *) malloc(sizeof(int) * num_threads);
    ThreadArg args = {
        .io_type = mode, .num_ops = num_ops, .num_iters = num_iter, .op_size = op_size
    };

    // Start timer
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i) {
        tids[i] = uthread_create(thread_io, &args);
        if (tids[i] == -1) {
            std::cerr << "uthread_create\n";
            exit(1);
        }
    }

    for (int i = 0; i < num_threads; ++i) {
        if (uthread_join(tids[i], nullptr) != 0) {
            std::cerr << "uthread_join\n";
            exit(1);
        }
    }

    // Stop timer
    auto end = std::chrono::high_resolution_clock::now();
    double dur = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << "Test IO with " << num_threads << " threads and " << num_ops
              << " IO operations per threads" << " with size " << op_size << " bytes\n"
              << "completed in: " << dur << " ms" << std::endl;

    return dur;
}

// Update file for next I/O operation
void reset_file(const char *message) {
#ifdef DEBUG
    // Move file offset to the end
    if (lseek(filedes, 0, SEEK_END) == -1) {
        std::cerr << "Failed to reset file: ";
        perror("lseek");
        exit(1);
    }
    // Write message to buffer
    snprintf(buf, 512, "%s", message);
    int length = strlen(buf);
    // Write to file
    if (write(filedes, buf, length) != length) {
        std::cerr << "Failed to reset file: ";
        perror("write");
        exit(1);
    }
    offset += length;
#else
    (void) message;
    if (ftruncate(filedes, 0) == -1) {
        perror("ftruncate");
        exit(1);
    }
    offset = 0;
#endif
}

int main(int argc, char *argv[]) {
    /*
    command:
    make run-io NTHREADS=10 NOPS=100 OPSIZE=4096 NITER=1000000 QUANTUM=10000
    */
    if (argc != 6) {
        // clang-format off
        std::cerr << "Usage: ./ioperformance <num_threads> <num_ops> <op_size> <num_iters> <quantum>\n";
        // clang-format on
        exit(1);
    }

    const int num_threads = atoi(argv[1]);
    const int num_ops = atoi(argv[2]);
    const size_t op_size = atoi(argv[3]);
    const int num_iters = atoi(argv[4]);
    buf = (char *) malloc(sizeof(char) * op_size);
    if (!buf) {
        perror("malloc");
        exit(1);
    }

    // Initialize uthread library
    uthread_init(atoi(argv[5]));

    filedes = open("ioperformance.txt", O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (filedes == -1) {
        perror("open");
        exit(1);
    }

    IOType async_type = ASYNC;
    IOType sync_type = SYNC;

    double async, sync;

    /**
     * run_test(
     *  number of threads,
     *  number of io operation,
     *  size of io operation,
     *  number of workload iterations,
     * )
     */
    std::cout << "Testing with " << num_iters << " iterations in the workload:\n";

    reset_file("Test 1: (async)\n");
    std::cout << "Testing async IO performance\n";
    async = run_test(num_threads, num_ops, async_type, op_size, num_iters);

    reset_file("\nTest 2: (sync)\n");
    std::cout << "Testing sync Performance...\n";
    sync = run_test(num_threads, num_ops, sync_type, op_size, num_iters);

    double ratio = sync / async;
    if (ratio > 1) {
        std::cout << "Performance ratio: async is " << ratio << " times faster\n";
    } else {
        std::cout << "Performance ratio: async is " << ratio << " times slower\n";
    }

    // Exit uthread library
    uthread_exit(nullptr);
    return 1;
}
