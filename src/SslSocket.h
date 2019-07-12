//
// Created by vhyz on 19-7-12.
//

#ifndef REQUESTS_SSLSOCKET_H
#define REQUESTS_SSLSOCKET_H

#include <openssl/err.h>
#include <openssl/ssl.h>
#include <string>
#include "Buffer.h"
#include "Socket.h"

class SslSocket : public Socket {
   private:
    SSL* ssl;

    static SSL_CTX* ctx;

    void sslInit();

   public:
    SslSocket(const char* ip, int port);

    SslSocket(const std::string& ip, int port);

    bool connect() override;

    void freeSsl();
};

#endif