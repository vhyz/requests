//
// Created by vhyz on 19-5-12.
//

#include "Socket.h"
#include <strings.h>
#include <zconf.h>

void Socket::send(const std::string &s) { send(s.c_str(), s.size()); }

void Socket::send(const char *msg, ssize_t n) {
    buffer.writeNBytes((void *)msg, n);
}

int Socket::recv(char *p, ssize_t n) { return buffer.readBuffer(p, n); }

std::string Socket::recv() {
    char buf[MAXLINE];
    int n = buffer.readBuffer(buf, MAXLINE);
    return std::string(buf, n);
}

Socket::Socket(const std::string &addr, int port)
    : Socket(addr.c_str(), port) {}

Socket::Socket(const char *addr, int port) {
    sockfd_init(addr, port);
    auto readCallBack =
        std::bind(::read, sockfd, std::placeholders::_1, std::placeholders::_2);
    auto writeCallBack = std::bind(::write, sockfd, std::placeholders::_1,
                                   std::placeholders::_2);
    buffer.setCallBack(readCallBack, writeCallBack);
}

bool Socket::connect() {
    if (::connect(sockfd, (sockaddr *)&sockaddrIn, sizeof sockaddrIn) == 0)
        return true;
    else
        return false;
}

void Socket::shutdownClose() { close(sockfd); }

void Socket::sockfd_init(const char *addr, int port) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&sockaddrIn, sizeof sockaddrIn);
    sockaddrIn.sin_family = AF_INET;
    sockaddrIn.sin_port = htons(port);
    inet_pton(AF_INET, addr, &sockaddrIn.sin_addr);
}

std::string Socket::readLine() {
    std::string res;
    char buf[MAXLINE];
    while (true) {
        int n = buffer.readLine(buf, MAXLINE);
        if (n == 0 || n == -1)
            break;
        res += std::string(buf, n - 1);
        if (buf[n - 1] == '\n') {
            if (*res.rbegin() == '\r')
                res.pop_back();
            break;
        }
    }
    return res;
}

std::string Socket::readNBytes(int n) {
    /*
     * C++17 std::string::data() is char*,it can be modified
     */
    std::string s(n, 0);
    buffer.readNBytes(s.data(), n);
    return s;
}

int Socket::readNBytes(char *p, size_t n) { return buffer.readNBytes(p, n); }
