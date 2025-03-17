#include "SpinLock.h"

SpinLock::SpinLock() {
    // Nothing to do
}

// Acquire the lock. Spin until the lock is acquired if the lock is already held
void SpinLock::lock() {
    while (atomic_value.test_and_set());
}

// Unlock the lock
void SpinLock::unlock() {
    atomic_value.clear();
}
