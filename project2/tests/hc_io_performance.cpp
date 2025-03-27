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

    std::cout << "Test IO with " << num_threads << " threads and " << num_ops
              << " IO operations per threads" << " with size " << op_size << " bytes\n"
              << "completed in: " << dur << " ms" << std::endl;

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

void full_test(int num_threads, int num_ops, size_t op_size, int num_iters) {
    IOType async_type = ASYNC;
    IOType sync_type = SYNC;

    double async = 0, sync = 0;

    std::cout << "=============================================================================\n";
    std::cout << "Testing with " << num_iters << " iterations in the workload:\n";

    for (int i = 0; i < NUM_ITER; i++) {
        std::cout << "Testing async IO performance...\n";
        async += run_test(num_threads, num_ops, async_type, op_size, num_iters);

        std::cout << "Testing sync IO performance...\n";
        sync += run_test(num_threads, num_ops, sync_type, op_size, num_iters);
    }

    std::cout << "Average async time: " << async / NUM_ITER << std::endl;
    std::cout << "Average sync time: " << sync / NUM_ITER << std::endl;

    double ratio = sync / async;

    if (ratio > 1) {
        std::cout << "Performance ratio: async is " << ratio << " times faster\n";
    } else {
        std::cout << "Performance ratio: async is " << ratio << " times slower\n";
    }
}

int main() {
    // Initialize uthread library
    uthread_init(10000);

    filedes = open("ioperformance.txt", O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if (filedes == -1) {
        perror("open");
        exit(1);
    }

    full_test(10, 5, 512, 50000);
    full_test(10, 5, 4096, 50000);
    full_test(10, 5, 8192, 50000);
    full_test(10, 5, 32768, 50000);
    full_test(10, 5, 65536, 50000);

    // Exit uthread library
    uthread_exit(nullptr);
    return 1;
}
