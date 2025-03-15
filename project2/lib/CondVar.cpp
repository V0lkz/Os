#include "CondVar.h"

#include <cassert>

#include "uthread_private.h"

CondVar::CondVar() {
    // Nothing to do
}

// Release the lock and block this thread atomically. Thread is woken up when
// signalled or broadcasted
void CondVar::wait(Lock &lock) {
    // Ensure thread has lock when calling wait
    assert(lock.held);
    disableInterrupts();
    // Add running thread to condition variable queue
    running->setState(BLOCK);
    queue.push(running);
    // Release the lock while interrupts are disabled
    lock._unlock();
    // Switch to another thread
    switchThreads();
    // Re-acquire lock
    lock.lock();
    // Interrupts already re-enabled
}

// Wake up a blocked thread if any is waiting
void CondVar::signal() {
    disableInterrupts();
    if (!queue.empty()) {
        // Remove a thread from queue
        TCB *next = queue.front();
        queue.pop();
        // Signal to run next thread??
        lock->_signal(next);
    }
    enableInterrupts();
}
