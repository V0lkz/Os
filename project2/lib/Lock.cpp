#include "Lock.h"
#include "uthread_private.h"
#include <cassert>

Lock::Lock() : held(false)
{
    // Nothing to do
}

// Attempt to acquire lock. Grab lock if available, otherwise thread is
// blocked until the lock becomes available
void Lock::lock()
{
    // TODO
}

// Unlock the lock. Wake up a blocked thread if any is waiting to acquire the
// lock and hand off the lock
void Lock::unlock()
{
    // TODO
}

// Unlock the lock while interrupts have already been disabled
// NOTE: This function should NOT be used by user code. It is only to be used
//       by uthread library code
void Lock::_unlock()
{
    // TODO
}

// Let the lock know that it should switch to this thread after the lock has
// been released (following Mesa semantics)
void Lock::_signal(TCB *tcb)
{
    // TODO
}
