#include "http.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../lib/async_io.h"
#include "../../lib/debug.cpp"

#define BUFSIZE 512

const char *get_mime_type(const char *file_extension) {
    if (strcmp(".txt", file_extension) == 0) {
        return "text/plain";
    } else if (strcmp(".html", file_extension) == 0) {
        return "text/html";
    } else if (strcmp(".jpg", file_extension) == 0) {
        return "image/jpeg";
    } else if (strcmp(".png", file_extension) == 0) {
        return "image/png";
    } else if (strcmp(".pdf", file_extension) == 0) {
        return "application/pdf";
    } else if (strcmp(".mp3", file_extension) == 0) {
        return "audio/mpeg";
    }

    return NULL;
}

int read_http_request(int fd, char *resource_name) {
    // Create buffer and variables for read
    char buf[BUFSIZE];
    char *saveptr = buf;
    int nbytes, offset = 0;
    // Read in the first line of http request (and extra)
    if ((nbytes = async_read(fd, buf, BUFSIZE, offset)) > 0) {
        // Extract the resoure name given by the second word
        strtok_r(saveptr, " ", &saveptr);
        char *token = strtok_r(saveptr, " ", &saveptr);
        strcat(resource_name, token);
        // Read in the rest of the http request
        while (nbytes == BUFSIZE) {
            PRINT("Thread %d reading in rest of http request\n", uthread_self())
            offset += nbytes;
            nbytes = async_read(fd, buf, BUFSIZE, offset);
        }
    }
    // Check if read errored
    if (nbytes == -1) {
        perror("read");
        return -1;
    }
    return 0;
}

int write_http_response(int fd, const char *resource_path) {
    // Create buffer for write
    char buf[BUFSIZE];
    memset(buf, 0, BUFSIZE);
    // Call stat to get file properties and check if resource path is valid
    struct stat statbuf;
    if (stat(resource_path, &statbuf) == -1) {
        // Check if stat failed from something other than ENOENT
        if (errno != ENOENT) {
            perror("stat");
            return -1;
        }
        // Otherwise resource path doesn't exist
        // Write the response header to buffer
        int ret_val = snprintf(buf, BUFSIZE, "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n");
        if (ret_val < 0) {
            fprintf(stderr, "snprintf failed\n");
            return -1;
        }
        // Write the response header to socket
        if (async_write(fd, buf, strlen(buf), 0) == -1) {
            perror("write");
            return -1;
        }
        return 0;
    }
    // Otherwise resource path exists
    // Extract file extension from resource path
    char *extension = NULL;
    strcpy(buf, resource_path);
    for (int i = strlen(resource_path); i > 0; i--) {
        if (buf[i] == '.') {
            extension = buf + i;
            break;
        }
    }
    // Check for no extension
    if (extension == NULL) {
        // Do something
        return 0;
    }
    // Get Content-Type and Content-Length
    long file_size = statbuf.st_size;
    const char *mime_type = get_mime_type(extension);
    if (mime_type == NULL) {
        fprintf(stderr, "Failed to get content-type: invalid file extension\n");
        return -1;
    }
    // Write the repsonse header to buffer
    int ret_val =
        snprintf(buf, BUFSIZE, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n",
                 mime_type, file_size);
    if (ret_val < 0) {
        fprintf(stderr, "snprintf failed\n");
        return -1;
    }
    // Write the response header to socket
    int header_len = strlen(buf);
    if (async_write(fd, buf, header_len, 0) == -1) {
        perror("write");
        return -1;
    }
    // Open the resource file
    int resource_fd = open(resource_path, O_RDONLY, S_IRUSR);
    if (resource_fd == -1) {
        perror("open");
        return -1;
    }
    // Write response body
    int nbytes, offset = 0;
    while ((nbytes = async_read(resource_fd, buf, BUFSIZE, offset)) > 0) {
        // Write contents of buffer into socket
        if (async_write(fd, buf, nbytes, header_len + offset) == -1) {
            perror("write");
            close(resource_fd);
            return -1;
        }
        // Add bytes read to offset
        offset += nbytes;
    }
    // Check if read errored
    if (nbytes == -1) {
        perror("read");
        close(resource_fd);
        return -1;
    }
    // Close resource file
    if (close(resource_fd) == -1) {
        perror("close");
        return -1;
    }
    return 0;
}
