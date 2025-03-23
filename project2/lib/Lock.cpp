#include "Lock.h"

#include <cassert>

#include "uthread_private.h"

#ifdef DEBUG
#define debug(x) std::cerr << x << std::endl;
#else
#define debug(x) (void) (x)
#endif

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
        // Switch to another thread
        switchThreads();
    }
    // Otherwise set held to true
    else {
        held = true;
    }
    std::cerr << "Lock acquired by " << running->getId() << std::endl;
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
        std::cerr << "signaled\n";
    }
    // Check if there are waiting entrance threads
    else if (!entrance_queue.empty()) {
        TCB *next = entrance_queue.front();
        entrance_queue.pop();
        next->setState(READY);
        addToReady(next);
        std::cerr << "entrance\n";
    }
    // Otherwise no waiting threads
    else {
        held = false;
        std::cerr << "Lock released by " << running->getId() << std::endl;
    }
}

// Let the lock know that it should switch to this thread after the lock has
// been released (following Mesa semantics)
void Lock::_signal(TCB *tcb) {
    // Add TCB to the signaled queue
    signaled_queue.push(tcb);
}
