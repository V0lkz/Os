#ifndef ASYNC_SOCKET_H
#define ASYNC_SOCKET_H

#define POLL_TIMEOUT 1    // Timeout in ms

int async_accept(int sockfd, struct sockaddr *addr, unsigned int *addrlen);

#endif    // ASYNC_SOCKET_H
