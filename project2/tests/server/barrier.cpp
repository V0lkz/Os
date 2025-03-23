// Authors: John Kolb, Matthew Dorow
// Modified by Anlei Chen
// SPDX-License-Identifier: GPL-3.0-or-later

#include "barrier.h"

#include <exception>

#include "../../lib/CondVar.h"
#include "../../lib/Lock.h"
#include "../../lib/uthread.h"

static Lock lock;
static CondVar waiter_cv;
static int n_waiters = 0;

int barrier(void) {
    try {
        lock.lock();
        // Check if thread is the final waiter
        if (n_waiters == CONCURRENCY_DEGREE - 1) {
            waiter_cv.broadcast();
            n_waiters = 0;
        }
        // Otherwise add to waiting list
        else {
            n_waiters++;
            waiter_cv.wait(lock);
        }
        lock.unlock();
    } catch (const std::exception &e) {
        std::cerr << "barrier: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
