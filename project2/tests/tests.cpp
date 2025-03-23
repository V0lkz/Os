#include <cstdlib>
#include <iostream>

#include "../lib/CondVar.h"
#include "../lib/Lock.h"
#include "../lib/uthread.h"

enum tests {
    MUTEX_LOCK = 1,
    SPIN_LOCK,
    COND_VAR,
    MULTI_COND_VAR,
    ASYNC_IO
};

/* Test 1: Mutex Lock */

#define NUM_THREADS_T1 5
#define NUM_ITER_T1 5

static volatile short wait = 0;
static Lock lock_t1;

static int values_t1[NUM_ITER_T1 * NUM_THREADS_T1];
static int counter_t1 = 0;

void *thread_mutex_lock(void *arg) {
    int tid = uthread_self();
    for (int i = 0; i < NUM_ITER_T1; i++) {
        lock_t1.lock();
        values_t1[counter_t1++] = tid;
        // Busy waiting with lock
        while (++wait)
            ;
        lock_t1.unlock();
    }
    return (void *) (long) tid;
}

int test_mutex_lock() {
    // Create threads
    int threads[NUM_THREADS_T1];
    for (int i = 0; i < NUM_THREADS_T1; i++) {
        uthread_create(thread_mutex_lock, NULL);
    }
    // Join threads
    int total_expected = 0;
    for (int i = 0; i < NUM_THREADS_T1; i++) {
        void *temp = nullptr;
        uthread_join(threads[i], &temp);
        total_expected += ((long) temp * NUM_ITER_T1);
    }
    // Check for correct results
    if (counter_t1 != NUM_ITER_T1 * NUM_THREADS_T1) {
        std::cerr << "Counter is incorrect" << std::endl;
        return -1;
    }
    int total_actual = 0;
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

int test_spin_lock() {
    return 0;
}

/* Test 3: Condition Variable */

int test_cond_var() {
    return 0;
}

/* Test 4: Multiple Condition Variables */

int test_multi_cond_var() {
    return 0;
}

/* Test 5: Asynchronus I/O */

int test_async_io() {
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
        std::cerr << "Mutex lock test passed!" << std::endl;
    }
    if (test_all || testnum == SPIN_LOCK) {
        if (test_spin_lock() != 0) {
            std::cerr << "Spinlock test failed!" << std::endl;
            exit(1);
        }
    }
    if (test_all || testnum == COND_VAR) {
        if (test_cond_var() != 0) {
            std::cerr << "Condition variable test failed!" << std::endl;
            exit(1);
        }
    }
    if (test_all || testnum == MULTI_COND_VAR) {
        if (test_multi_cond_var() != 0) {
            std::cerr << "Multiple condition variables test failed!" << std::endl;
            exit(1);
        }
    }
    if (test_all || testnum == ASYNC_IO) {
        if (test_async_io() != 0) {
            std::cerr << "Asynchronus i/o test failed!" << std::endl;
            exit(1);
        }
    }

    // Exit uthread library
    uthread_exit(nullptr);
}
