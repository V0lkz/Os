#include <chrono>
#include <iostream>
#include "../lib/Lock.h"
#include "../lib/SpinLock.h"
#include "../lib/uthread.h"

struct ThreadArgs {
    int num_iterations;
    int inner_loop_size;
};


Lock global_lock;
SpinLock spin_lock;
uint64_t shared_counter = 0;


void add_workload(){
    for (int i = 0; i < 100000; ++i) {
        volatile int x = i * i;
    }
}


void *critical_section_with_mutexlock(void *arg) {
    int iterations_size = ((ThreadArgs *) arg)->num_iterations;
    int loop_size = ((ThreadArgs *)arg)->inner_loop_size;
    add_workload();
    for (int i = 0; i < iterations_size; i++) {
        global_lock.lock();
        for (int j = 0; j < loop_size; j++) {  
            shared_counter++;         // Critical section
        }  
       
        global_lock.unlock();
    }
    return nullptr;
}


void *critical_section_with_spinlock(void *arg) {
    int iterations_size = ((ThreadArgs *) arg)->num_iterations;
    int loop_size = ((ThreadArgs *)arg)->inner_loop_size;
    add_workload();
    for (int i = 0; i < iterations_size; i++) {
        spin_lock.lock();
        for (int j = 0; j < loop_size; j++) {  
            shared_counter++;        // Critical section
        }
        spin_lock.unlock();
    }
    return nullptr;
}


void run_test(void *(*lock_func)(void *), const std::string &lock_type,int num_threads,int num_iterations ,int num_inner_loop) {
    // Start timer
    auto start_time = std::chrono::high_resolution_clock::now();


    int tids[num_threads];
    ThreadArgs arg = { num_iterations, num_inner_loop };
    for (int i = 0; i < num_threads; ++i) {
        tids[i] = uthread_create(lock_func, &arg);
        if (tids[i] == -1) {
            std::cerr << "uthread_create\n";
        }
    }


    for (int i = 0; i < num_threads; ++i) {
        if (uthread_join(tids[i], nullptr) != 0) {
            std::cerr << "uthread_join\n";
        }
    }


    // Stop timer
    auto end_time = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double, std::milli>(end_time - start_time).count();


    std::cout << "Lock: "<< lock_type << " with " << num_threads << " threads, "
          << num_iterations << " iterations/thread, "
          << num_inner_loop << " ops/critical-section"
          << " completed in " << duration << " ms " << std::endl;
}


int main() {
    uthread_init(1000);


   
    std::cout << "Test 1\n";
    //run_test(function, (string) locktype, (int) number of threads, (int) number of iterations, (int) inner loop size);
    run_test(critical_section_with_lock, "Lock", 2, 100, 100);


    // Reset shared counter
    shared_counter = 0;
    run_test(critical_section_with_spinlock, "SpinLock", 2, 100, 100);




    std::cout << "Test 2\n";
    run_test(critical_section_with_lock, "Lock", 2, 100, 1000);
   
    // Reset shared counter
    shared_counter = 0;


    run_test(critical_section_with_spinlock, "SpinLock", 2, 100, 1000);


    std::cout << "Test 3\n";
    run_test(critical_section_with_lock, "Lock", 2, 100, 12000);
   
    // Reset shared counter
    shared_counter = 0;


    run_test(critical_section_with_spinlock, "SpinLock", 2, 100, 12000);


    std::cout << "Test 2\n";
    run_test(critical_section_with_lock, "Lock", 20, 100, 10000);
   
    // Reset shared counter
    shared_counter = 0;


    run_test(critical_section_with_spinlock, "SpinLock", 20, 100, 10000);


    uthread_exit(nullptr);
    return 0;
}
