//
// Created by vhyz on 19-5-12.
//

#ifndef REQUESTS_SOCKET_H
#define REQUESTS_SOCKET_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include "Buffer.h"

/*
 * Socket : base class of ClientSocket and ServerSocket
 */
class Socket {
protected:
    int sockfd;
    Buffer buffer;
    sockaddr_in sockaddrIn;

    void sockfd_init(const char *addr, int port);

public:
    Socket(const char *addr, int port);

    Socket(const std::string &addr, int port);

/*
 *
 */
    virtual void shutdownClose();

    virtual void send(const std::string &s);

    virtual void send(const char *msg, ssize_t n);

    virtual std::string recv();

    virtual int recv(char *p, ssize_t n);

    virtual std::string readNBytes(int n);

    virtual std::string readLine();

    virtual int readNBytes(char *p, size_t n);
};


#endif //REQUESTS_SOCKET_H
