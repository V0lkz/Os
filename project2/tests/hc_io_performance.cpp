#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <vector>

#include "../lib/async_io.h"
#include "../lib/uthread.h"

#define NUM_ITER 5
#define QUANTUM 10000

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

typedef struct wtime {
    char *test;
    double async;
    double sync;
} wtime_t;

static int filedes;
static off_t offset;

volatile long x = 0;
static char buf[65536];

void add_workload(int num_iters) {
    for (int i = 0; i < num_iters; i++) {
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
            if (async_write(filedes, buf, length, offset) != length) {
                perror("async_write");
            }
        }
        add_workload(num_iters);
    }
    return nullptr;
}

// Run thread test
double run_test(int num_threads, int num_ops, IOType mode, size_t op_size, int num_iters) {
    // Create thread array and args
    int *tids = (int *) malloc(sizeof(int) * num_threads);
    ThreadArg args = {
        .io_type = mode, .num_ops = num_ops, .num_iters = num_iters, .op_size = op_size
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

    std::cout << "completed in: " << dur << " ms" << std::endl;

    free(tids);
    return dur;
}

// Update file for next I/O operation
void reset_file(void) {
    if (ftruncate(filedes, 0) == -1) {
        perror("ftruncate");
        exit(1);
    }
    offset = 0;
}

wtime_t full_test(int num_threads, int num_ops, size_t op_size, int num_iters) {
    IOType async_type = ASYNC;
    IOType sync_type = SYNC;

    double async = 0, sync = 0;

    std::cout << "=============================================================================\n";
    std::cout << "Testing with " << num_iters << " iterations in the workload:\n";
    std::cout << "I/O with " << num_threads << " threads and " << num_ops
              << " I/O operations per threads" << " with size " << op_size << " bytes:\n";

    for (int i = 0; i < NUM_ITER; i++) {
        std::cout << "async: ";
        async += run_test(num_threads, num_ops, async_type, op_size, num_iters);

        std::cout << "sync: ";
        sync += run_test(num_threads, num_ops, sync_type, op_size, num_iters);
    }

    char *buf = (char *) malloc(sizeof(char) * 128);
    sprintf(buf, "%d %d %ld %d", num_threads, num_ops, op_size, num_iters);
    wtime_t time = { .test = buf, .async = async / NUM_ITER, .sync = sync / NUM_ITER };

    std::cout << "Average async time: " << time.async << std::endl;
    std::cout << "Average sync time: " << time.sync << std::endl;

    double ratio = sync / async;

    if (ratio > 1) {
        std::cout << "Performance ratio: async is " << ratio << " times faster\n";
    } else {
        std::cout << "Performance ratio: async is " << ratio << " times slower\n";
    }

    return time;
}

int main() {
    // Initialize uthread library
    uthread_init(QUANTUM);

    // Open file to write to
    filedes = open("ioperformance.txt", O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (filedes == -1) {
        perror("open");
        exit(1);
    }

    wtime_t times[10];

    times[0] = full_test(10, 5, 512, 50000);
    times[1] = full_test(10, 5, 4096, 50000);
    times[2] = full_test(10, 5, 8192, 50000);
    times[3] = full_test(10, 5, 32768, 50000);
    times[4] = full_test(10, 5, 65536, 50000);

    times[5] = full_test(10, 5, 4096, 10000);
    times[6] = full_test(10, 5, 4096, 50000);
    times[7] = full_test(10, 5, 4096, 100000);
    times[8] = full_test(10, 5, 4096, 500000);
    times[9] = full_test(10, 5, 4096, 10000000);

    std::cout << "=============================================================================\n";
    std::cout << "Summary:" << std::endl;

    for (wtime_t time : times) {
        std::cout << "Test: " << time.test << " " << QUANTUM << std::endl;
        std::cout << " async: " << time.async << std::endl;
        std::cout << " sync: " << time.sync << std::endl;
        free(time.test);
    }

    // Close file
    if (close(filedes) == -1) {
        perror("close");
    }

    // Exit uthread library
    uthread_exit(nullptr);
    return 1;
}
