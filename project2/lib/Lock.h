#ifndef LOCK_H
#define LOCK_H

#include "TCB.h"
#include <queue>

// Synchronization lock
class Lock
{
public:
    Lock();

    // Attempt to acquire lock. Grab lock if available, otherwise thread is
    // blocked until the lock becomes available
    void lock();

    // Unlock the lock. Wake up a blocked thread if any is waiting to acquire the
    // lock and hand off the lock
    void unlock();

private:
    bool held;                        // true if the lock is held, false otherwise
    std::queue<TCB *> entrance_queue; // queue of threads waiting to acquire the lock
    std::queue<TCB *> signaled_queue; // queue of threads that signaled and are waiting to reacquire the lock

    // Unlock the lock while interrupts have already been disabled
    // NOTE: Assumes interrupts are disabled
    void _unlock();

    // Let the lock know that it should switch to this thread after the lock has
    // been released (following Mesa semantics)
    // NOTE: Assumes interrupts are disabled
    void _signal(TCB *tcb);

    // Allow condition variable class access to Lock private members
    // NOTE: CondVar should only use _unlock() and _signal() private functions
    //       (should not access private variables directly)
    friend class CondVar;
};

#endif // LOCK_H
