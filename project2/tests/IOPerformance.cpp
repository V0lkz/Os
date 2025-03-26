#include <fcntl.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <iostream>
#include <vector>

#include "../lib/async_io.h"
#include "../lib/uthread.h"

#define MAX_LEN 80

enum IOType {
    SYNC,
    ASYNC
};

struct ThreadArg {
    IOType io_type;
    int num_ops;
};

static int fd;
static int offset = 0;
static volatile long x = 0;

void add_workload() {
    for (int i = 0; i < 1000000; ++i) {
        x += i * i;
    }
}

// Thread Function
void *thread_io(void *args) {
    ThreadArg *params = (ThreadArg *) args;
    IOType mode = params->io_type;
    int num_ops = params->num_ops;
    int tid = uthread_self();

    // Write thread id as a string
    char buf[MAX_LEN];
    memset(buf, 0, MAX_LEN);
    snprintf(buf, MAX_LEN, "%d ", tid);

    int length = strlen(buf);

    if (offset == 0) {
        offset -= length;
    }

    for (int i = 0; i < num_ops; ++i) {
        // Write I/O to completion
        if (mode == SYNC) {
            if (write(fd, buf, length) == -1) {
                perror("write");
            }
            offset += length;
        }
        // Allow other threads to run while waiting for I/O
        else {
            // Reserve space in file for id
            offset += length;
            if (async_write(fd, buf, length, offset - length) == -1) {
                perror("async_write");
            };
        }
        // add_workload();
    }
    return nullptr;
}

void run_test(int num_threads, int num_ops, IOType mode) {
    auto start = std::chrono::high_resolution_clock::now();

    int *tids = (int *) malloc(sizeof(int) * num_threads);
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
    free(tids);
}

bool reset_file(const char *message) {
    // Move file offset to the end
    if (lseek(fd, 0, SEEK_END) == -1) {
        perror("lseek");
        return false;
    }

    // Write new line at the end of the file
    char buf[MAX_LEN];
    snprintf(buf, MAX_LEN, "%s", message);
    int length = strlen(buf);
    // Write to file
    write(fd, buf, length);
    offset += length;

    return true;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: ./ioperformance <num_threads> <num_ops> <quantum>" << std::endl;
        exit(1);
    }

    const int num_threads = atoi(argv[1]);
    const int num_ops = atoi(argv[2]);

    // Initialize uthread library
    uthread_init(atoi(argv[3]));

    IOType async_type = ASYNC;
    IOType sync_type = SYNC;

    // Open file to test on
    fd = open("ioperformance.txt", O_CREAT | O_TRUNC | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("open");
        exit(1);
    }

    // run_test((int) number of threads, (int) number of IO operations/thread, IOType))

    std::cout << "Test IO with 1 thread and " << num_ops << " IO operations per thread:\n";

    if (!reset_file("Test 1: (async)\n")) {
        std::cerr << "Failed to reset file" << std::endl;
    }

    std::cout << "Testing async IO performance...\n";
    run_test(1, num_ops, async_type);

    if (!reset_file("\nTest 2: (sync)\n")) {
        std::cerr << "Failed to reset file" << std::endl;
    }

    std::cout << "Testing sync Performance...\n";
    run_test(1, num_ops, sync_type);

    std::cout << "Test IO with " << num_threads << " threads and 1 IO operation per threads:\n";

    if (!reset_file("\nTest 3: (async)\n")) {
        std::cerr << "Failed to reset file" << std::endl;
    }

    std::cout << "Testing async IO performance...\n";
    run_test(num_threads, 1, async_type);

    if (!reset_file("\nTest 4: (sync)\n")) {
        std::cerr << "Failed to reset file" << std::endl;
    }

    std::cout << "Testing sync Performance...\n";
    run_test(num_threads, 1, sync_type);

    std::cout << "Test IO with " << num_threads << " threads and " << num_ops
              << " IO operations per threads:\n";

    if (!reset_file("\nTest 5: (async)\n")) {
        std::cerr << "Failed to reset file" << std::endl;
    }

    std::cout << "Testing async IO performance...\n";
    run_test(num_threads, num_ops, async_type);

    if (!reset_file("\nTest 6: (sync)\n")) {
        std::cerr << "Failed to reset file" << std::endl;
    }

    std::cout << "Testing sync Performance...\n";
    run_test(num_threads, num_ops, sync_type);

    uthread_exit(nullptr);
    return 1;
}
