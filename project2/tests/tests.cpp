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

int test_mutex_lock() {
    return 0;
}

int test_spin_lock() {
    return 0;
}

int test_cond_var() {
    return 0;
}

int test_multi_cond_var() {
    return 0;
}

int test_async_io() {
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc > 3) {
        std::cerr << "Usage ./tests <testnum> <quantum_usecs>" << std::endl;
        exit(1);
    }

    const int testnum = atoi(argv[2]);
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
    }
    if (test_all || testnum == SPIN_LOCK) {
        if (test_spin_lock() != 0) {
            std::cerr << "Spinlock test failed!" << std::endl;
            exit(1);
        }
    }
    if (test_all || testnum == COND_VAR) {
        if (test_cond_var() != 0) {
            std::cerr << "Cond Var test failed!" << std::endl;
            exit(1);
        }
    }
    if (test_all || testnum == MULTI_COND_VAR) {
        if (test_multi_cond_var() != 0) {
            std::cerr << "Multi Cond Var test failed!" << std::endl;
            exit(1);
        }
    }
    if (test_all || testnum == ASYNC_IO) {
        if (test_async_io() != 0) {
            std::cerr << "Async io test failed!" << std::endl;
            exit(1);
        }
    }

    // Exit uthread library
    uthread_exit(nullptr);
}
