#include <fcntl.h>
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
};

static int pipe_fds[2];

void add_workload() {
    for (int i = 0; i < 10000000; ++i) {
        volatile int x = i * i;
    }
}

void *thread_io(void *args) {
    ThreadArg *params = (ThreadArg *) args;
    IOType mode = params->io_type;
    int num_ops = params->num_ops;
    int tid = uthread_self();

    add_workload();
    for (int i = 0; i < num_ops; ++i) {
        if (mode == SYNC) {
            // write to completion
            if (write(pipe_fds[1], &tid, sizeof(tid)) != sizeof(tid)) {
                perror("write");
            }
        } else {
            // allows other threads to run while in io
            if (async_write(pipe_fds[1], &tid, sizeof(tid), 0) == -1) {
                perror("async_write");
            };
        }
    }
    return nullptr;
}

void run_test(int num_threads, int num_ops, IOType mode) {
    auto start = std::chrono::high_resolution_clock::now();

    int tids[num_threads];
    ThreadArg args = { mode, num_ops };
    for (int i = 0; i < num_threads; ++i) {
        tids[i] = uthread_create(thread_io, &args);
        if (tids[i] == -1)
            std::cerr << "uthread_create\n";
    }

    for (int i = 0; i < num_threads; ++i) {
        if (uthread_join(tids[i], nullptr) != 0) {
            std::cerr << "uthread_join\n";
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    double dur = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << " completed in " << dur << " ms\n";
}

bool reset_pipe() {
    close(pipe_fds[0]);
    close(pipe_fds[1]);

    if (pipe(pipe_fds) == -1) {
        perror("pipe");
        return false;
    }
    return true;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ./ioperformance <quantum>" << std::endl;
        exit(1);
    }
    uthread_init(atoi(argv[1]));

    IOType async_type = ASYNC;
    IOType sync_type = SYNC;

    // run_test((int)number of threads,(int)number of IO operations/thread, IOType))
    std::cout << "Test IO with 2 threads and 500 IO operations per threads:";
    std::cout << "Testing async IO performance\n";
    run_test(2, 500, async_type);

    std::cout << "Testing sync Performance...\n";
    if (reset_pipe()) {
        run_test(2, 500, sync_type);
    }

    std::cout << "Test IO with 100 threads and 1 IO operations per threads:";
    std::cout << "Testing async IO performance\n";
    if (reset_pipe()) {
        run_test(100, 1, async_type);
    }

    std::cout << "Testing sync Performance...\n";
    if (reset_pipe()) {
        run_test(100, 1, sync_type);
    }

    std::cout << "Test IO with 10 threads and 50 IO operations per threads:";
    std::cout << "Testing async IO performance\n";
    if (reset_pipe()) {
        run_test(10, 50, async_type);
    }

    std::cout << "Testing sync Performance...\n";
    if (reset_pipe()) {
        run_test(10, 50, sync_type);
    }

    uthread_exit(nullptr);
    return 1;
}
