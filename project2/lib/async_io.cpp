#include "async_io.h"

#include <aio.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "debug.cpp"
#include "uthread.h"

// Carry out an asynchronous read request where this thread will be blocked
// while servicing the read but other ready threads will be scheduled
// Input:
// - fd: File descriptor
// - buf: Buffer to store read in
// - count: Number of bytes to read
// - offset: File offset to start at
// Output:
// - Number of bytes read on success, -1 on failure
ssize_t async_read(int fd, void *buf, size_t count, int offset) {
    // clang-format off
    struct aiocb async_read_req = {
        .aio_fildes = fd,
        .aio_buf = buf,
        .aio_nbytes = count,
        .aio_offset = offset
    };
    // clang-format on

    // Return immediately if initialization fails
    if (aio_read(&async_read_req) != 0) {
        perror("aio_read");
        return -1;
    }

    // Polling until completion
    int ret_val;
    while ((ret_val = aio_error(&async_read_req)) == EINPROGRESS) {
        S_PRINT(5000, "Thread %d waiting in read\n", uthread_self());
        uthread_yield();
    }
    // Check if there is an error
    if (ret_val != 0) {
        fprintf(stderr, "aio_error: %s\n", strerror(ret_val));
        return -1;
    }

    // Return I/O result
    PRINT("Thread %d ready to read\n", uthread_self());
    return aio_return(&async_read_req);
}

// Carry out an asynchronous write request where this thread will be blocked
// while servicing the write but other ready threads will be scheduled
// Input:
// - fd: File descriptor
// - buf: Buffer containing data to write to file
// - count: Number of bytes to write
// - offset: File offset to start at
// Output:
// - Number of bytes written on success, -1 on failure
ssize_t async_write(int fd, void *buf, size_t count, int offset) {
    // clang-format off
    struct aiocb async_write_req = {
        .aio_fildes = fd,
        .aio_buf = buf,
        .aio_nbytes = count,
        .aio_offset = offset
    };
    // clang-format on

    // Return immediately if initization fails
    if (aio_write(&async_write_req) == -1) {
        perror("aio_write");
        return -1;
    }

    // Polling until completion
    int ret_val;
    while ((ret_val = aio_error(&async_write_req)) == EINPROGRESS) {
        S_PRINT(5000, "Thread %d waiting in write\n", uthread_self());
        uthread_yield();
    }
    // Check if there is an error
    if (ret_val != 0) {
        fprintf(stderr, "aio_error: %s\n", strerror(ret_val));
        return -1;
    }

    // Return I/O result
    PRINT("Thread %d ready to write\n", uthread_self());
    return aio_return(&async_write_req);
}
