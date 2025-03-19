#include <iostream>
#include <chrono>
#include <vector>
#include "../lib/uthread.h"
#include "../lib/Lock.h"
#include "../lib/SpinLock.h"

#define NUM_THREADS 10
#define NUM_ITERATIONS 1000000

Lock global_lock;
SpinLock spin_lock;
int shared_counter = 0;

void* critical_section_with_lock(void*) {
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        global_lock.lock();
        shared_counter++;  // Critical section
        global_lock.unlock();
    }
    return nullptr;
}

void* critical_section_with_spinlock(void*) {
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        spin_lock.lock();
        shared_counter++;  // Critical section
        spin_lock.unlock();
    }
    return nullptr;
}

void run_test(void* (*lock_func)(void*), const std::string &lock_type) {
    shared_counter = 0;

    auto start_time = std::chrono::high_resolution_clock::now();

    std::vector<int> threads;
    for (int i = 0; i < NUM_THREADS; i++) {
        threads.push_back(uthread_create(lock_func, nullptr));
    }

    for (int thread : threads) {
        uthread_join(thread, nullptr);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    std::cout << lock_type << " completed in " << duration << " ms, final counter: " << shared_counter << std::endl;
}

int main() {
    uthread_init(1000000);  

    std::cout << "Testing Lock Performance...\n";
    run_test(critical_section_with_lock, "Lock");

    std::cout << "Testing SpinLock Performance...\n";
    run_test(critical_section_with_spinlock, "SpinLock");

    return 0;
}
