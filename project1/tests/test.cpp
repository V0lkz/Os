#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "../lib/uthread.h"

void *func2(void *arg) {
    return (void *) ((long) (uthread_self()));
}

void *func1(void *arg) {
    std::cout << "Thread Running: " << uthread_self() << std::endl;
    return NULL;
}

void test(int i) {
    std::cout << "Test: " << i << std::endl;
}

int main(int argc, char *argv[]) {
    /* Initialize the default time slice (only overridden if passed in) */
    int quantum_usecs = 1e6;

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
    std::cout << "id: " << tid3 << " ret: " << (long) (ret3);

    uthread_exit(NULL);
    return 0;
}
