//
// Created by vhyz on 19-5-12.
//

#include "ClientSokcet.h"

ClientSocket::ClientSocket(const char *addr, int port) : Socket(addr, port) {
    connect(sockfd, (sockaddr *) &sockaddrIn, sizeof sockaddrIn);
}

ClientSocket::ClientSocket(const std::string &addr, int port) : ClientSocket(addr.c_str(), port) {}
