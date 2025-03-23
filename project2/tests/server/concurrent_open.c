// Authors: John Kolb, Matthew Dorow
// Modified by Anlei Chen
// SPDX-License-Identifier: GPL-3.0-or-later
#define _GNU_SOURCE

#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "barrier.h"

/*
 * Version of the open syscall that will only allow threads to proceed once a
 * sufficient number of threads have initiated an open syscall.
 * Specifically, it blocks all calling threads until 'CONCURRENCY_DEGREE'
 * threads have made a call to open().
 * This is a (probably inelegant) way to check if a program is really capable
 * of 'CONCURRENCY_DEGREE' threads of execution.
 */

// Returns true if the pathname provided is to a server file, false otherwise
int is_server_file(const char *pathname) {
    return (strncmp(SERVER_FILE_PREFIX, pathname, strlen(SERVER_FILE_PREFIX)) == 0);
}

int open(const char *pathname, int flags, ...) {
    int (*open_orig)(const char *pathname, int flags);
    open_orig = dlsym(RTLD_NEXT, "open");    // Get pointer to real open
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
