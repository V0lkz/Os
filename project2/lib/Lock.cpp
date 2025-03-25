#include "Lock.h"

#include <cassert>

#include "debug.cpp"
#include "uthread_private.h"

Lock::Lock() : held(false) {
    // Nothing to do
}

// Attempt to acquire lock. Grab lock if available, otherwise thread is
// blocked until the lock becomes available
void Lock::lock() {
    disableInterrupts();
    // Check if lock is held
    if (held) {
        // Add running thread to entrance queue
        running->setState(BLOCK);
        entrance_queue.push(running);
        PRINT("Thread %d added to entrance queue\n", running->getId());
        // Switch to another thread
        switchThreads();
    }
    // Otherwise set held to true
    else {
        held = true;
    }
    PRINT("Lock acquired by %d\n", running->getId());
    enableInterrupts();
}

// Unlock the lock. Wake up a blocked thread if any is waiting to acquire the
// lock and hand off the lock
void Lock::unlock() {
    disableInterrupts();
    // Call interrupt disabled version
    _unlock();
    enableInterrupts();
}

// Unlock the lock while interrupts have already been disabled
// NOTE: This function should NOT be used by user code. It is only to be used
//       by uthread library code
void Lock::_unlock() {
    // Check if there are waiting signaled threads
    if (!signaled_queue.empty()) {
        TCB *next = signaled_queue.front();
        signaled_queue.pop();
        next->setState(READY);
        addToReady(next);
        PRINT("Thread %d removed from signaled queue by thread %d\n", next->getId(),
              running->getId());
    }
    // Check if there are waiting entrance threads
    else if (!entrance_queue.empty()) {
        TCB *next = entrance_queue.front();
        entrance_queue.pop();
        next->setState(READY);
        addToReady(next);
        PRINT("Thread %d removed from entrance queue by thread %d\n", next->getId(),
              running->getId());
    }
    // Otherwise no waiting threads
    else {
        held = false;
        PRINT("Lock released by thread %d\n", running->getId());
    }
}

// Let the lock know that it should switch to this thread after the lock has
// been released (following Mesa semantics)
void Lock::_signal(TCB *tcb) {
    // Add TCB to the signaled queue
    signaled_queue.push(tcb);
    PRINT("Thread %d signaled by thread %d\n", tcb->getId(), running->getId());
}
