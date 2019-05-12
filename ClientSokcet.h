//
// Created by vhyz on 19-5-12.
//

#ifndef REQUESTS_CLIENTSOKCET_H
#define REQUESTS_CLIENTSOKCET_H

#include "Buffer.h"
#include "Socket.h"


class ClientSocket : public Socket {
public:
    ClientSocket(const char *addr, int port);

    ClientSocket(const std::string &addr, int port);

};

#endif //REQUESTS_CLIENTSOKCET_H
