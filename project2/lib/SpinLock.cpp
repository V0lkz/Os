#include "SpinLock.h"

#include "uthread_private.h"

SpinLock::SpinLock() {
    // Nothing to do
}

// Acquire the lock. Spin until the lock is acquired if the lock is already held
void SpinLock::lock() {
    PRINT("Thread %d aquiring spinlock\n", running->getId());
    while (atomic_value.test_and_set());
    PRINT("Spinlock acquired by thread %d\n", running->getId());
}

// Unlock the lock
void SpinLock::unlock() {
    PRINT("Spinlock released by thread %d\n", running->getId());
    atomic_value.clear();
}
