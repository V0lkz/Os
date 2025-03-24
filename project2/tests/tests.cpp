#include <cstdlib>
#include <iostream>

#include "../lib/CondVar.h"
#include "../lib/Lock.h"
#include "../lib/SpinLock.h"
#include "../lib/uthread.h"

enum tests {
    MUTEX_LOCK = 1,
    SPIN_LOCK,
    COND_VAR,
    MULTI_COND_VAR,
    ASYNC_IO
};

// Busy waiting counter
static volatile unsigned int wait = 0;

/* Testing Setup Functions */

#define NUM_THREADS 5

static void *t_results[NUM_THREADS];
static int threads[NUM_THREADS];

int testing_setup(void *(*thread_func)(void *), void *args) {
    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        threads[i] = uthread_create(thread_func, args);
        if (threads[i] == -1) {
            std::cerr << "uthread_create" << std::endl;
            return -1;
        }
    }
    return 0;
}

int testing_cleanup() {
    // Join threads
    for (int i = 0; i < NUM_THREADS; i++) {
        if (uthread_join(threads[i], &t_results[i]) != 0) {
            std::cerr << "uthread_join" << std::endl;
            return -1;
        }
    }
    return 0;
}

/* Test 1: Mutex Lock */

#define NUM_ITER_T1 5

static Lock lock_t1;

static int values_t1[NUM_ITER_T1 * NUM_THREADS];
static int counter_t1 = 0;

void *thread_mutex_lock(void *args) {
    (void) args;
    int tid = uthread_self();
    for (int i = 0; i < NUM_ITER_T1; i++) {
        lock_t1.lock();
        values_t1[counter_t1++] = tid;
        while (++wait &= 0x7FFFFFF);
        lock_t1.unlock();
    }
    std::cout << "Thread " << tid << " finished" << std::endl;
    return (void *) (long) tid;
}

// Tests Lock::lock() and Lock::unlock()
int test_mutex_lock() {
    std::cout << "Starting mutex lock test..." << std::endl;
    // Setup threads
    if (testing_setup(thread_mutex_lock, nullptr) != 0) {
        return -1;
    }
    // Join threads
    if (testing_cleanup() != 0) {
        return -1;
    }
    // Check for correct results
    if (counter_t1 != NUM_ITER_T1 * NUM_THREADS) {
        std::cerr << "Counter is incorrect" << std::endl;
        return -1;
    }
    long total_expected = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        total_expected += (long) t_results[i] * NUM_ITER_T1;
    }
    long total_actual = 0;
    for (int i = 0; i < counter_t1; i++) {
        total_actual += values_t1[i];
    }
    if (total_actual != total_expected) {
        std::cerr << "Total sum is incorrect" << std::endl;
        return -1;
    }
    return 0;
}

/* Test 2: SpinLock */

#define NUM_ITER_T2 5

SpinLock spinlock_t2;

static int values_t2[NUM_ITER_T2 * NUM_THREADS];
static int counter_t2 = 0;

void *thread_spin_lock(void *args) {
    (void) args;
    int tid = uthread_self();
    for (int i = 0; i < NUM_ITER_T1; i++) {
        spinlock_t2.lock();
        values_t2[counter_t2++] = tid;
        while (++wait &= 0x3FFFFFF);
        spinlock_t2.unlock();
    }
    std::cout << "Thread " << tid << " finished" << std::endl;
    return (void *) (long) tid;
}

// Tests Spinlock::lock() and Spinlock::unlock()
int test_spin_lock() {
    std::cout << "Starting spinlock test..." << std::endl;
    // Setup threads
    if (testing_setup(thread_spin_lock, nullptr) != 0) {
        return -1;
    }
    // Join threads
    if (testing_cleanup() != 0) {
        return -1;
    }
    // Check for correct results
    if (counter_t2 != NUM_ITER_T2 * NUM_THREADS) {
        std::cerr << "Counter is incorrect" << std::endl;
        return -1;
    }
    long total_expected = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        total_expected += (long) t_results[i] * NUM_ITER_T2;
    }
    long total_actual = 0;
    for (int i = 0; i < counter_t2; i++) {
        total_actual += values_t2[i];
    }
    if (total_actual != total_expected) {
        std::cerr << "Total sum is incorrect" << std::endl;
        return -1;
    }
    return 0;
}

/* Test 3: Condition Variable */

static Lock lock_t3;
static CondVar cv_t3;

static int waiting_threads = 0;

void *thread_cond_var(void *args) {
    (void) args;
    // Busy waiting
    while (++wait);
    lock_t3.lock();
    // Broadcast to all threads if its the final thread
    if (waiting_threads == NUM_THREADS - 1) {
        cv_t3.broadcast();
    }
    // Otherwise, check in at barrier
    else {
        waiting_threads++;
        cv_t3.wait(lock_t3);
    }
    lock_t3.unlock();
    std::cout << "Thread " << uthread_self() << " exited barrier" << std::endl;
    return (void *) (long) waiting_threads;
}

// Tests CondVar::wait() and CondVar::broadcast()
int test_cond_var() {
    std::cout << "Starting condition variable test..." << std::endl;
    // Setup threads
    if (testing_setup(thread_cond_var, nullptr) != 0) {
        return -1;
    }
    // Join threads
    if (testing_cleanup() != 0) {
        return -1;
    }
    // Check for correct results
    for (int i = 0; i < NUM_THREADS; i++) {
        if ((long) t_results[i] != (NUM_THREADS - 1)) {
            std::cerr << "Thread count is incorrect" << std::endl;
            return -1;
        }
    }
    return 0;
}

/* Test 4: Multiple Condition Variables */

static Lock lock_t4;
static CondVar full_cv;
static CondVar empty_cv;

void *thread_multi_cond_var(void *args) {
    (void) args;

    return nullptr;
}

int test_multi_cond_var() {
    std::cout << "Starting multiple condition variable test..." << std::endl;
    // Setup threads
    if (testing_setup(thread_multi_cond_var, nullptr) != 0) {
        return -1;
    }
    // Join threads
    if (testing_cleanup() != 0) {
        return -1;
    }
    return 0;
}

/* Test 5: Asynchronus I/O */

int test_async_io() {
    std::cout << "Starting asynchronus i/o test..." << std::endl;
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        std::cerr << "Usage ./test <testnum> <quantum_usecs>" << std::endl;
        exit(1);
    }

    const int testnum = atoi(argv[1]);
    const int quantum_usecs = atoi(argv[2]);
    const bool test_all = (testnum <= 0 ? true : false);

    // Initialize uthread library
    if (uthread_init(quantum_usecs) != 0) {
        std::cerr << "uthread_init" << std::endl;
        exit(1);
    }

    // Run tests
    if (test_all || testnum == MUTEX_LOCK) {
        if (test_mutex_lock() != 0) {
            std::cerr << "Mutex lock test failed!" << std::endl;
            exit(1);
        }
        std::cout << "Mutex lock test passed!" << std::endl;
    }
    if (test_all || testnum == SPIN_LOCK) {
        if (test_spin_lock() != 0) {
            std::cerr << "Spinlock test failed!" << std::endl;
            exit(1);
        }
        std::cout << "Spinlock test passed!" << std::endl;
    }
    if (test_all || testnum == COND_VAR) {
        if (test_cond_var() != 0) {
            std::cerr << "Condition variable test failed!" << std::endl;
            exit(1);
        }
        std::cout << "Condition variable test passed!" << std::endl;
    }
    if (test_all || testnum == MULTI_COND_VAR) {
        if (test_multi_cond_var() != 0) {
            std::cerr << "Multiple condition variables test failed!" << std::endl;
            exit(1);
        }
        std::cout << "Multiple condition variables test passed!" << std::endl;
    }
    if (test_all || testnum == ASYNC_IO) {
        if (test_async_io() != 0) {
            std::cerr << "Asynchronus I/O test failed!" << std::endl;
            exit(1);
        }
        std::cout << "Asynchronus I/O test passed!" << std::endl;
    }

    // Exit uthread library
    uthread_exit(nullptr);
}
