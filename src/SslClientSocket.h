//
// Created by vhyz on 19-5-12.
//

#ifndef REQUESTS_SSLCLIENTSOCKET_H
#define REQUESTS_SSLCLIENTSOCKET_H

#include "Socket.h"
#include "Buffer.h"
#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>


class SslClientSocket : public Socket {
public:
    SslClientSocket(const char *addr, int port);

    SslClientSocket(const std::string &addr, int port);

    void shutdownClose() override;

    void send(const std::string &s) override;

    void send(const char *msg, ssize_t n) override;

    int recv(char *p, ssize_t n) override;

    std::string recv() override;

    std::string readNBytes(int n) override;

    int readNBytes(char *p, size_t n) override;

    std::string readLine() override;

private:
    ssize_t sslReadBuffer(char *usrbuf, size_t n);

    ssize_t sslReadLine(void *usrbuf, size_t maxlen);

    ssize_t sslReadNBytes(void *usrbuf, size_t n);

    ssize_t sslWriteNBytes(void *usrbuf, size_t n);

private:
    SSL *ssl;

    static SSL_CTX *ctx;

    static bool isSslInit;

    static void sslInit();

    static void freeCtx();

    static void createCtx();

};

#endif //REQUESTS_SSLCLIENTSOCKET_H
