#include "connection_queue.h"

#include <stdio.h>
#include <string.h>

#include <exception>

int connection_queue_init(connection_queue_t *queue) {
    // Initialize the rest of the connection queue bookkeeping variables
    memset(&queue->client_fds, 0, sizeof(int) * CAPACITY);
    queue->length = 0;
    queue->read_idx = 0;
    queue->write_idx = 0;
    queue->shutdown = 0;
    return 0;
}

int connection_queue_enqueue(connection_queue_t *queue, int connection_fd) {
    try {
        // Acquire mutex lock
        queue->lock.lock();
        // Wait until queue is no longer full
        while (queue->length == CAPACITY) {
            // Wait for condition variable
            queue->full_cv.wait(queue->lock);
            // Check if queue is shutdown
            if (queue->shutdown == 1) {
                queue->lock.unlock();
                return -1;
            }
        }
        // Enqueue new connection file desciptor
        queue->client_fds[queue->write_idx++] = connection_fd;
        queue->write_idx = queue->write_idx % CAPACITY;
        queue->length++;
        // Signal condition variable
        queue->empty_cv.signal();
        // Release mutex lock
        queue->lock.unlock();
    } catch (const std::exception &e) {
        std::cerr << "connection_queue_enqueue: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}

int connection_queue_dequeue(connection_queue_t *queue) {
    int client_fd;
    try {
        // Acquire mutex lock
        queue->lock.lock();
        // Wait until queue is no longer empty
        while (queue->length == 0) {
            // Wait for condition variable
            queue->empty_cv.wait(queue->lock);
            // Check if queue is shutdown
            if (queue->shutdown == 1) {
                queue->lock.unlock();
                return -1;
            }
        }
        // Dequeue connection file descriptor
        client_fd = queue->client_fds[queue->read_idx++];
        queue->read_idx = queue->read_idx % CAPACITY;
        queue->length--;
        // Signal condition variable
        queue->full_cv.signal();
        // Release mutex lock
        queue->lock.unlock();
    } catch (const std::exception &e) {
        std::cerr << "connection_queue_dequeue: " << e.what() << std::endl;
        return -1;
    }
    return client_fd;
}

int connection_queue_shutdown(connection_queue_t *queue) {
    // Set queue shutdown
    queue->shutdown = 1;
    // Broadcast all condition variables to wake up threads
    try {
        queue->empty_cv.broadcast();
        queue->full_cv.broadcast();
    } catch (const std::exception &e) {
        std::cerr << "connection_queue_shutdown: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
