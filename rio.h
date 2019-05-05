//
// Created by vhyz on 19-5-1.
//

#ifndef REQUESTS_RIO_H
#define REQUESTS_RIO_H

#include <sys/types.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define RIO_BUFSIZE 8196

struct rio_t {
    union {
        int rio_fd;
        SSL* ssl;
    };
    bool is_ssl;
    int rio_cnt;
    char *rio_bufptr;
    char rio_buf[RIO_BUFSIZE];
};

void rio_init(rio_t *rp, int fd);

ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n);

ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen);

ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n);

ssize_t rio_writen(int fd, void *usrbuf, size_t n);


#endif //REQUESTS_RIO_H
