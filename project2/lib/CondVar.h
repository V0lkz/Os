#ifndef COND_VAR_H
#define COND_VAR_H

#include "TCB.h"
#include "Lock.h"
#include <queue>

// Synchronization condition variable
// NOTE: Follows Mesa semantics
class CondVar
{
public:
    CondVar();

    // Release the lock and block this thread atomically. Thread is woken up when
    // signalled or broadcasted
    void wait(Lock &lock);

    // Following Mesa semantics, Wake up a blocked thread if any is waiting
    void signal();

    // Following Mesa semantics, Wake up all blocked threads if any are waiting
    void broadcast();

private:
    Lock *lock;              // Pointer to lock being used with condition variable
    std::queue<TCB *> queue; // queue of threads waiting for a signal
};

#endif // COND_VAR_H
