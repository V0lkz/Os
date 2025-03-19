#include <chrono>
#include <iostream>
#include <vector>

#include "../lib/Lock.h"
#include "../lib/SpinLock.h"
#include "../lib/uthread.h"

#define NUM_THREADS 10
#define NUM_ITERATIONS 100000

Lock global_lock;
SpinLock spin_lock;
int shared_counter = 0;

void *critical_section_with_lock(void *) {
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        global_lock.lock();
        shared_counter++;    // Critical section
        global_lock.unlock();
    }
    return nullptr;
}

void *critical_section_with_spinlock(void *) {
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        spin_lock.lock();
        shared_counter++;    // Critical section
        spin_lock.unlock();
    }
    return nullptr;
}

void run_test(void *(*lock_func)(void *), const std::string &lock_type) {
    // Start timer
    auto start_time = std::chrono::high_resolution_clock::now();

    // Create threads
    std::vector<int> threads;
    for (int i = 0, tid = -1; i < NUM_THREADS; i++) {
        if ((tid = uthread_create(lock_func, nullptr)) == -1) {
            std::cerr << "uthread_create\n";
        }
        threads.push_back(tid);
    }

    // Join threads
    for (int thread : threads) {
        if (uthread_join(thread, nullptr) != 0) {
            std::cerr << "uthread_join\n";
        }
    }

    // End timer
    auto end_time = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    std::cout << lock_type << " completed in " << duration
              << " ms, final counter: " << shared_counter << std::endl;
}

int main() {
    uthread_init(1000);

    std::cout << "Testing Lock Performance...\n";
    run_test(critical_section_with_lock, "Lock");

    // Reset shared counter
    shared_counter = 0;

    std::cout << "Testing SpinLock Performance...\n";
    run_test(critical_section_with_spinlock, "SpinLock");

    uthread_exit(nullptr);
    return 0;
}
