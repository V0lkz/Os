#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "../lib/uthread.h"

uthread_once_t uthread_once_control = UTHREAD_ONCE_INIT;
int once_count = 0;

void init_function() {
    once_count++;
}

void *thread_once_function(void *arg) {
    uthread_once(&uthread_once_control, init_function);
    return NULL;
}

void *func7(void *arg) {
    for (size_t i = 0; i < 5; i++) {
        for (size_t j = 0; j < 100000; j++) {
            ;    // Nop
        }
        uthread_yield();
    }
    return NULL;
}

void *func6(void *arg) {
    size_t i = 0;
    for (; i < 10000; i++) {
        for (size_t j = 0; j < 10000; j++) {
            ;    // Nop
        }
    }
    return (void *) (i);
}

void *func4(void *arg) {
    long tid = reinterpret_cast<long>(arg);
    uthread_exit((void *) tid);
    return NULL;
}

void *func2(void *arg) {
    int tid = uthread_self();
    return (void *) ((long) tid);
}

void *func1(void *arg) {
    std::cout << "Thread Running: " << uthread_self() << std::endl;
    return NULL;
}

static int quantum_usecs = 100000;    // 100 ms quantum default

void test(int i) {
    std::cout << "---------- Test: " << i << " ----------" << std::endl;
    // Test 1
    if (i == 1) {
        std::cout << "Test init, create, self, and join" << std::endl;
        std::cout << "Initializing library" << std::endl;
        if (uthread_init(quantum_usecs) != 0) {
            std::cerr << "uthread_init" << std::endl;
            exit(1);
        }
        std::cout << "Creating thread" << std::endl;
        int tid1;
        if (tid1 = uthread_create(func1, NULL) == -1) {
            std::cerr << "uthread_create" << std::endl;
            exit(1);
        }
        std::cout << "Joining thread" << std::endl;
        if (uthread_join(tid1, NULL) != 0) {
            std::cerr << "uthread_join" << std::endl;
            exit(1);
        }
        std::cout << uthread_self() << std::endl;
    }
    // Test 2
    else if (i == 2) {
        std::cout << "Test creating and joining multiple threads" << std::endl;
        int tid2[3];
        for (int i = 0; i < 3; i++) {
            tid2[i] = uthread_create(func1, NULL);
            if (tid2[i] == -1) {
                std::cerr << "uthread_create" << std::endl;
                exit(1);
            }
        }
        std::cout << "Joining threads" << std::endl;
        for (int i = 0; i < 3; i++) {
            if (uthread_join(tid2[i], NULL) != 0) {
                std::cerr << "uthread_join" << std::endl;
                exit(1);
            }
        }
    }
    // Test 3
    else if (i == 3) {
        void *ret3 = nullptr;
        int tid3 = -1;
        std::cout << "Test return value from stub" << std::endl;
        if ((tid3 = uthread_create(func2, &ret3)) != 0) {
            std::cerr << "uthread_create" << std::endl;
            exit(1);
        }
        std::cout << "Joining thread" << std::endl;
        if (uthread_join(tid3, &ret3) != 0) {
            std::cerr << "uthread_join" << std::endl;
            exit(1);
        }
        std::cout << "Thread: " << tid3 << " return: " << (long) (ret3) << std::endl;
    }
    // Test 4
    else if (i == 4) {
        void *ret4 = nullptr;
        int tid4[3];
        std::cout << "Test multiple return value from uthread_exit" << std::endl;
        for (long i = 0; i < 3; i++) {
            if ((tid4[i] = uthread_create(func4, (void *) i)) == -1) {
                std::cerr << "uthread_create" << std::endl;
                exit(1);
            }
        }
        std::cout << "Joining threads" << std::endl;
        for (int i = 0; i < 3; i++) {
            if (uthread_join(tid4[i], &ret4) != 0) {
                std::cerr << "uthread_join" << std::endl;
                exit(1);
            }
            std::cout << "Thread: " << i << " return: " << (long) (ret4) << std::endl;
        }
    }
    // Test 5
    else if (i == 5) {
        std::cout << "Test suspend, resume, and yield" << std::endl;
        int tid5_1 = uthread_create(func1, NULL);
        int tid5_2 = uthread_create(func1, NULL);
        if (tid5_1 == -1 || tid5_2 == -1) {
            std::cerr << "uthread_create" << std::endl;
            exit(1);
        }
        std::cout << "Suspending Thread: " << tid5_1 << std::endl;
        if (uthread_suspend(tid5_1) != 0) {
            std::cerr << "uthread_suspend" << std::endl;
            exit(1);
        }
        std::cout << "Yielding main thread" << std::endl;
        if (uthread_yield() != 0) {
            std::cerr << "uthread_yield" << std::endl;
            exit(1);
        }
        std::cout << "Joining Thread: " << tid5_2 << std::endl;
        if (uthread_join(tid5_2, NULL) != 0) {
            std::cerr << "uthread_join" << std::endl;
            exit(1);
        }
        std::cout << "Resuming Thread: " << tid5_1 << std::endl;
        if (uthread_resume(tid5_1) != 0) {
            std::cerr << "uthread_resume" << std::endl;
            exit(1);
        }
        std::cout << "Joining Thread: " << tid5_1 << std::endl;
        if (uthread_join(tid5_1, NULL) != 0) {
            std::cerr << "uthread_join" << std::endl;
            exit(1);
        }
    }
    // Test 6
    else if (i == 6) {
        std::cout << "Test get quantum and interrupt timer" << std::endl;
        void *retval = nullptr;
        int tid6[5];
        for (int i = 0; i < 5; i++) {
            if ((tid6[i] = uthread_create(func6, NULL)) == -1) {
                std::cerr << "uthread_create" << std::endl;
                exit(1);
            }
        }
        std::cout << "Joining Threads" << std::endl;
        for (int i = 0; i < 5; i++) {
            if (uthread_join(tid6[i], &retval) != 0) {
                std::cerr << "uthread_join" << std::endl;
                exit(1);
            }
            // Note: quantum should be -1, because tid5[i] has been joined
            std::cout << "Thread: " << tid6[i] << ", Loop: " << (long) (retval)
                      << ", Quantum: " << uthread_get_quantums(tid6[i]) << std::endl;
        }
        std::cout << uthread_get_total_quantums() << std::endl;
    }
    // Test 7
    else if (i == 7) {
        std::cout << "Test voluntary yield inside thread" << std::endl;
        int tid6[3];
        for (int i = 0; i < 3; i++) {
            if ((tid6[i] = uthread_create(func7, NULL)) == -1) {
                std::cerr << "uthread_create" << std::endl;
                exit(1);
            }
        }
        std::cout << "Yielding main thread" << std::endl;
        if (uthread_yield() != 0) {
            std::cerr << "uthread_yield" << std::endl;
            exit(1);
        }
        std::cout << "Joining Threads" << std::endl;
        for (int i = 0; i < 3; i++) {
            std::cout << uthread_get_quantums(tid6[i]) << std::endl;
            if (uthread_join(tid6[i], NULL) != 0) {
                std::cerr << "uthread_join" << std::endl;
                exit(1);
            }
        }
        std::cout << "Total quantums: " << uthread_get_total_quantums() << std::endl;
    }
    // Test 8
    else if (i == 8) {
        std::cout << "Test uthread_once" << std::endl;
        int tid7[4];
        for (int i = 0; i < 4; i++) {
            tid7[i] = uthread_create(thread_once_function, NULL);
        }
        for (int i = 0; i < 4; i++) {
            uthread_join(tid7[i], NULL);
        }
        if (once_count == 1) {
            std::cout << "Test 7 completed" << std::endl;
        } else {
            std::cerr << "Test 7 failed " << std::endl;
        }
    }
    std::cout << "Test " << i << " Completed!" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char *argv[]) {
    int quantum_usecs = 100000;    // 100 ms quantums

    // Run all tests
    if (argc == 1) {
        for (int i = 1; i <= 7; i++) {
            test(i);
        }
    }
    // Run selected test
    else if (argc == 2) {
        test(atoi(argv[1]));
    }
    // Unexpected inputs
    else {
        std::cout << "Usage: \"./test x\" to run test x or \"./test\" to run all tests\n";
        return 0;
    }

    uthread_exit(NULL);
    return 0;
}
