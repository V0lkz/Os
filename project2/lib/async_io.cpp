#include "async_io.h"

#include <aio.h>
#include <errno.h>

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
    /* Consider how this should be used */
    // clang-format off
    struct aiocb async_read_req = {
        .aio_fildes = fd,
        .aio_buf = buf,
        .aio_nbytes = count,
        .aio_offset = offset
    };
    
    //Returen immediately if initiation fails
    if(aio_read(&async_read_req) == -1){
        return -1; 
    }

    //Polling, wait for completion
    while(aio_error(&async_read_req) == EINPROGRESS){
        uthread_yield();
    }

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
    /* Consider how this should be used */
    struct aiocb async_write_req = {
        .aio_fildes = fd, .aio_buf = buf, .aio_nbytes = count, .aio_offset = offset
    };

    //Returen immediately if initiation fails
    if(aio_write(&async_write_req) == -1){
        return -1; 
    }

    //Polling, wait for completion
    while(aio_error(&async_write_req) == EINPROGRESS){
        uthread_yield();
    }

    return aio_return(&async_write_req);
}
