#include <chrono>
#include <iostream>

#include "../lib/Lock.h"
#include "../lib/SpinLock.h"
#include "../lib/uthread.h"

#define NUM_THREADS 10
#define NUM_ITERATIONS 1000000

Lock global_lock;
SpinLock spin_lock;
uint64_t shared_counter = 0;

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

    int tids[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; ++i) {
        tids[i] = uthread_create(lock_func, nullptr);
        if (tids[i] == -1) {
            std::cerr << "uthread_create\n";
        }
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        if (uthread_join(tids[i], nullptr) != 0) {
            std::cerr << "uthread_join\n";
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    std::cout << lock_type << " completed: " << duration
              << " ms, final counter: " << shared_counter << std::endl;
}

int main() {
    uthread_init(100000);

    std::cout << "Testing Lock Performance...\n";
    run_test(critical_section_with_lock, "Lock");

    // Reset shared counter
    shared_counter = 0;

    std::cout << "Testing SpinLock Performance...\n";
    run_test(critical_section_with_spinlock, "SpinLock");

    uthread_exit(nullptr);
    return 0;
}
