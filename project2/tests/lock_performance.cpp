#include <chrono>
#include <iostream>

#include "../lib/Lock.h"
#include "../lib/SpinLock.h"
#include "../lib/uthread.h"

struct ThreadArgs {
    int n_iterations;
    int n_loops;
    int workload;
};

Lock mutex_lock;
SpinLock spin_lock;
uint64_t shared_counter = 0;

volatile uint64_t x = 0;

void add_workload(int num_iters) {
    for (int i = 0; i < num_iters; i++) {
        x += i * i;
    }
}

void *critical_section_with_mutexlock(void *args) {
    ThreadArgs *params = (ThreadArgs *) args;
    int iterations_size = params->n_iterations;
    int loop_size = params->n_loops;
    int workload = params->workload;
    for (int i = 0; i < iterations_size; i++) {
        mutex_lock.lock();
        for (int j = 0; j < loop_size; j++) {
            shared_counter++;    // Critical section
        }
        mutex_lock.unlock();
        add_workload(workload);
    }
    return nullptr;
}

void *critical_section_with_spinlock(void *args) {
    ThreadArgs *params = (ThreadArgs *) args;
    int iterations_size = params->n_iterations;
    int loop_size = params->n_loops;
    int workload = params->workload;
    for (int i = 0; i < iterations_size; i++) {
        spin_lock.lock();
        for (int j = 0; j < loop_size; j++) {
            shared_counter++;    // Critical section
        }
        spin_lock.unlock();
        add_workload(workload);
    }
    return nullptr;
}

void run_test(void *(*lock_func)(void *), const std::string &lock_type, int nthreads, int niters,
              int nloops, int workload) {
    // Start timer
    auto start_time = std::chrono::high_resolution_clock::now();

    int *tids = (int *) malloc(sizeof(int) * nthreads);
    ThreadArgs args = { .n_iterations = niters, .n_loops = nloops, .workload = workload };

    for (int i = 0; i < nthreads; ++i) {
        tids[i] = uthread_create(lock_func, &args);
        if (tids[i] == -1) {
            std::cerr << "uthread_create\n";
        }
    }

    for (int i = 0; i < nthreads; ++i) {
        if (uthread_join(tids[i], nullptr) != 0) {
            std::cerr << "uthread_join\n";
        }
    }

    // Stop timer
    auto end_time = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    std::cout << lock_type << " with " << nthreads << " threads, " << niters
              << " iterations/thread, " << nloops << " ops/critical-section"
              << " completed in " << duration << " ms " << std::endl;

    free(tids);
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: ./lockperformance <nthreads> <iterations> <nloops> <workload> "
                     "<quantum>\n";
        exit(1);
    }

    const int num_threads = atoi(argv[1]);
    const int iterations = atoi(argv[2]);
    const int num_loops = atoi(argv[3]);
    const int workload = atoi(argv[4]);

    // Initialize thread library
    uthread_init(atoi(argv[5]));

    /**
     * run_test(
     *  thread function,
     *  lock type,
     *  number of threads,
     *  number of iterations,
     *  number of inner loops
     * )
     */

    std::cout << "==================== Test 1 ====================\n";

    std::cout << "Testing MutexLock Performance...\n";
    run_test(critical_section_with_mutexlock, "MutexLock", num_threads, iterations, 1, 0);

    // Reset shared counter
    shared_counter = 0;

    std::cout << "Testing SpinLock Performance...\n";
    run_test(critical_section_with_spinlock, "SpinLock", num_threads, iterations, 1, 0);

    // Reset shared counter
    shared_counter = 0;

    std::cout << "==================== Test 2 ====================\n";

    std::cout << "Testing MutexLock Performance...\n";
    run_test(critical_section_with_mutexlock, "MutexLock", num_threads, iterations, num_loops,
             workload);

    // Reset shared counter
    shared_counter = 0;

    std::cout << "Testing SpinLock Performance...\n";
    run_test(critical_section_with_spinlock, "SpinLock", num_threads, iterations, num_loops,
             workload);

    uthread_exit(nullptr);
    return 0;
}
