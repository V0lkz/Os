#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "../lib/uthread.h"

void *func5(void *arg) {
    long i = 0;
    long end = 100000000000000L * uthread_self();
    while (i < end) {
        i++;
    }
    return (void *) (i);
}

void *func2(void *arg) {
    return (void *) ((long) (uthread_self()));
}

void *func1(void *arg) {
    std::cout << "Thread Running: " << uthread_self() << std::endl;
    return NULL;
}

void test(int i) {
    std::cout << "---------- Test: " << i << " ----------" << std::endl;
}

int main(int argc, char *argv[]) {
    int quantum_usecs = 1000;    // 1 ms quantums

    void *retval = nullptr;

    test(1);    // Create one thread

    uthread_init(quantum_usecs);
    int tid1 = uthread_create(func1, NULL);
    uthread_join(tid1, NULL);

    std::cerr << std::endl;

    test(2);    // Create multiple threads

    int tid2[3];
    for (int i = 0; i < 3; i++) {
        tid2[i] = uthread_create(func1, NULL);
    }
    for (int i = 0; i < 3; i++) {
        uthread_join(tid2[i], NULL);
    }

    std::cerr << std::endl;

    test(3);    // Test return value

    void *ret3;
    int tid3 = uthread_create(func2, &ret3);
    std::cout << "id: " << tid3 << " ret: " << (long) (ret3) << std::endl;

    uthread_join(tid3, NULL);

    std::cerr << std::endl;

    test(4);    // Test suspend and resume

    int tid4_1 = uthread_create(func1, NULL);
    int tid4_2 = uthread_create(func1, NULL);

    std::cout << "Suspending Thread: " << tid4_1 << std::endl;
    uthread_suspend(tid4_1);

    std::cout << "Joining Thread: " << tid4_2 << std::endl;
    uthread_join(tid4_2, NULL);

    std::cout << "Resuming Thread: " << tid4_1 << std::endl;
    uthread_resume(tid4_1);

    std::cout << "Joining Thread: " << tid4_1 << std::endl;
    uthread_join(tid4_1, NULL);

    std::cout << "Test 4 Completed!" << std::endl;

    std::cerr << std::endl;

    test(5);    // Test interrupt timer

    int tid5[5];
    for (int i = 0; i < 5; i++) {
        tid5[i] = uthread_create(func5, NULL);
    }

    for (int i = 0; i < 5; i++) {
        void *retval;
        uthread_join(tid5[i], &retval);
        std::cout << "Thread: " << tid5[i] << ", Loop: " << (long) (retval)
                  << ", Quantum: " << uthread_get_quantums(tid5[i]) << std::endl;
    }
    std::cout << uthread_get_total_quantums() << std::endl;

    return 0;
}
