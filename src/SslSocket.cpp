//
// Created by vhyz on 19-7-12.
//

#include "SslSocket.h"

SslSocket::SslSocket(const std::string& ip, int port)
    : SslSocket(ip.c_str(), port) {}

SslSocket::SslSocket(const char* ip, int port) {
    sockfd_init(ip, port);
    sslInit();
    ssl = SSL_new(ctx);
    auto readCallBack =
        std::bind(SSL_read, ssl, std::placeholders::_1, std::placeholders::_2);
    auto writeCallBack =
        std::bind(SSL_write, ssl, std::placeholders::_1, std::placeholders::_2);
    buffer.setCallBack(readCallBack, writeCallBack);
}

SSL_CTX* SslSocket::ctx = nullptr;

void SslSocket::sslInit() {
    if (ctx == nullptr) {
        SSL_library_init();
        SSL_load_error_strings();
        ctx = SSL_CTX_new(TLS_client_method());
    }
}

void SslSocket::freeSsl() {
    if (ctx != nullptr) {
        SSL_CTX_free(ctx);
        ctx = nullptr;
    }
}

bool SslSocket::connect() {
    if (Socket::connect() == false)
        return false;
    SSL_set_fd(ssl, sockfd);
    if (SSL_connect(ssl) < 0)
        return false;
    return true;
}