#include "SpinLock.h"

#ifdef DEBUG
#include "uthread_private.h"
#define PRINT(format, ...) fprintf(stderr, format, __VA_ARGS__)
#else
#define PRINT(fstr, ...)    // DEBUG OFF
#endif

SpinLock::SpinLock() {
    // Nothing to do
}

// Acquire the lock. Spin until the lock is acquired if the lock is already held
void SpinLock::lock() {
    PRINT("Thread %d aquiring spinlock\n", running->getId());
    while (atomic_value.test_and_set());
    PRINT("Thread %d acquired spinlock\n", running->getId());
}

// Unlock the lock
void SpinLock::unlock() {
    PRINT("Thread %d released spinlock\n", running->getId());
    atomic_value.clear();
}
