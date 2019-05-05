//
// Created by vhyz on 19-5-1.
//

#ifndef REQUESTS_RIO_H
#define REQUESTS_RIO_H

#include <sys/types.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define RIO_BUFSIZE 8196

class rio_t {
public:
    int rio_fd;
    SSL *ssl;
    SSL_CTX *ctx;
    int rio_cnt;
    bool is_ssl;
    char *rio_bufptr;
    char rio_buf[RIO_BUFSIZE];

    rio_t() = default;

    void init(int fd, bool is_ssl);

    ssize_t rioRead(char *usrbuf, size_t n);

    ssize_t rioReadLine(void *usrbuf, size_t maxlen);

    ssize_t rioReadNBytes(void *usrbuf, size_t n);

    ssize_t rioWriten(void *usrbuf, size_t n);
};

#endif //REQUESTS_RIO_H
