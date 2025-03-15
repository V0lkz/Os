#include "CondVar.h"
#include "uthread_private.h"
#include <cassert>

CondVar::CondVar()
{
    // Nothing to do
}

// Release the lock and block this thread atomically. Thread is woken up when
// signalled or broadcasted
void CondVar::wait(Lock &lock)
{
    // TODO 
}

// Wake up a blocked thread if any is waiting
void CondVar::signal()
{
    // TODO
}
