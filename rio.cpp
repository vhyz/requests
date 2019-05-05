//
// Created by vhyz on 19-5-1.
//

#include "rio.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

void rio_t::init(int fd, bool is_ssl) {
    this->is_ssl = is_ssl;
    this->rio_fd = fd;
    this->rio_cnt = 0;
    this->rio_bufptr = rio_buf;
}

ssize_t rio_t::rioReadLine(void *usrbuf, size_t maxlen) {
    int n, rc;
    char c;
    char *ptr = static_cast<char *>(usrbuf);
    for (n = 1; n <= maxlen; ++n) {
        if ((rc = rioRead(&c, 1)) == 1) {
            *ptr++ = c;
            if (c == '\n')
                break;
        } else if (rc < 0) {
            return -1;
        } else {
            if (n == 1)
                return 0;  // EOF
            else
                break;
        }
    }
    *ptr = 0;
    return n;
}

ssize_t rio_t::rioRead(char *usrbuf, size_t n) {
    int cnt;

    while (rio_cnt <= 0) {
        if (is_ssl) {
            rio_cnt = SSL_read(ssl, rio_buf, sizeof(rio_buf));
        } else {
            rio_cnt = read(rio_fd, rio_buf, sizeof(rio_buf));
        }
        if (rio_cnt < 0) {
            if (errno != EINTR) return -1;
        } else if (rio_cnt == 0) {
            return 0;
        } else {
            rio_bufptr = rio_buf;
        }
    }
    cnt = rio_cnt < n ? rio_cnt : n;
    memcpy(usrbuf, rio_bufptr, cnt);
    rio_bufptr += cnt;
    rio_cnt -= cnt;
    return cnt;
}

ssize_t rio_t::rioWriten(void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nwrite;
    char *ptr = static_cast<char *>(usrbuf);

    while (nleft > 0) {
        if (is_ssl) {
            nwrite = SSL_write(ssl, ptr, nleft);
        } else {
            nwrite = write(rio_fd, ptr, nleft);
        }
        if (nwrite < 0) {
            if (errno != EINTR)
                return -1;
            else
                nwrite = 0;
        }
        nleft -= nwrite;
        ptr += nwrite;
    }

    return n - nleft;
}

ssize_t rio_t::rioReadNBytes(void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char *ptr = static_cast<char *>(usrbuf);

    while (nleft > 0) {
        if ((nread = rioRead(ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0;
            else
                return -1;
        } else if (nread == 0)
            break;
        nleft -= nread;
        ptr += nread;
    }
    return n - nleft;
}