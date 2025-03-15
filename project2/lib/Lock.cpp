#include "Lock.h"

#include <cassert>

#include "uthread_private.h"

Lock::Lock() : held(false) {
    // Nothing to do
}

// Attempt to acquire lock. Grab lock if available, otherwise thread is
// blocked until the lock becomes available
void Lock::lock() {
    // TCB pointer to next thread to run
    TCB *next;
    disableInterrupts();
    // Check if lock is held
    if (held) {
        // Add running thread to entrance queue
        running->setState(BLOCK);
        entrance_queue.push(running);
        // Pop from signaled queue
        if (!signaled_queue.empty()) {
            next = signaled_queue.front();
            signaled_queue.pop();
        }
        // Otherwise pop from entrance queue
        else {
            next = entrance_queue.front();
            entrance_queue.pop();
        }
        // Switch to next thread
        switchToThread(next);
    }
    // Otherwise set held to true
    else {
        held = true;
    }
    enableInterrupts();
}

// Unlock the lock. Wake up a blocked thread if any is waiting to acquire the
// lock and hand off the lock
void Lock::unlock() {
    // TODO
    disableInterrupts();
    // Acquire spin lock
    if ()
        // Release spin lock
        enableInterrupts();
}

// Unlock the lock while interrupts have already been disabled
// NOTE: This function should NOT be used by user code. It is only to be used
//       by uthread library code
void Lock::_unlock() {
    // TODO
}

// Let the lock know that it should switch to this thread after the lock has
// been released (following Mesa semantics)
void Lock::_signal(TCB *tcb) {
    // TODO
}
