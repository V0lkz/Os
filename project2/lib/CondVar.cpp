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
    *(this->lock) = lock;    // ?
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
    // Check if there are other waiting threads
    if (!queue.empty()) {
        // Remove thread from queue
        TCB *next = queue.front();
        queue.pop();
        // Add thread to signaled queue
        lock->_signal(next);
    }
    enableInterrupts();
}
