// Authors: John Kolb, Matthew Dorow
// Modified by Anlei Chen
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <exception>

#include "../../lib/CondVar.h"
#include "../../lib/Lock.h"
#include "../../lib/uthread.h"

#define SERVER_FILE_PREFIX "server_files/"
#define CONCURRENCY_DEGREE 5

static Lock lock;
static CondVar waiter_cv;
static int n_waiters = 0;

// Wait until 'CONCURRENCY_DEGREE' threads all have initiated barrier().
// Then, allow all of them to proceed.
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

// Returns true if the pathname provided is to a server file, false otherwise
int is_server_file(const char *pathname) {
    return (strncmp(SERVER_FILE_PREFIX, pathname, strlen(SERVER_FILE_PREFIX)) == 0);
}

/*
 * Version of the open syscall that will only allow threads to proceed once a
 * sufficient number of threads have initiated an open syscall.
 * Specifically, it blocks all calling threads until 'CONCURRENCY_DEGREE'
 * threads have made a call to open().
 * This is a (probably inelegant) way to check if a program is really capable
 * of 'CONCURRENCY_DEGREE' threads of execution.
 */
int open(const char *pathname, int flags, ...) {
    // Get pointer to real open
    int (*open_orig)(const char *pathname, int flags);
    open_orig = (int (*)(const char *, int)) dlsym(RTLD_NEXT, "open");
    char *error = dlerror();
    if (error != NULL) {
        fprintf(stderr, "dlsym: %s\n", error);
        return -1;
    }

    // If thread isn't opening a server file, let it proceed
    if (!is_server_file(pathname)) {
        return open_orig(pathname, flags);
    }

    // Otherwise, check in at the barrier
    int barrier_checkin = barrier();
    if (barrier_checkin != 0) {
        return -1;
    }

    return open_orig(pathname, flags);
}
