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

void rio_init(rio_t *rp, int fd) {
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

void rio_ssl_init(rio_t_ssl *rp, int fd) {
    const SSL_METHOD *method = SSLv23_client_method();
    rp->ssl = SSL_new(SSL_CTX_new(method));
    SSL_set_fd(rp->ssl, fd);
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}

ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n) {
    int cnt;

    while (rp->rio_cnt <= 0) {
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));

        if (rp->rio_cnt < 0) {
            if (errno != EINTR) return -1;
        } else if (rp->rio_cnt == 0) {
            return 0;
        } else {
            rp->rio_bufptr = rp->rio_buf;
        }
    }
    cnt = rp->rio_cnt < n ? rp->rio_cnt : n;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen) {
    int n, rc;
    char c;
    char *ptr = static_cast<char *>(usrbuf);
    for (n = 1; n <= maxlen; ++n) {
        if ((rc = rio_read(rp, &c, 1)) == 1) {
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

ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char *ptr = static_cast<char *>(usrbuf);

    while (nleft > 0) {
        if ((nread = rio_read(rp, ptr, nleft)) < 0) {
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

ssize_t rio_writen(int fd, void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nwrite;
    char *ptr = static_cast<char *>(usrbuf);

    while (nleft > 0) {
        if ((nwrite = write(fd, ptr, nleft)) < 0) {
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

ssize_t rio_readnb_s(rio_t_ssl *rp, void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char *ptr = static_cast<char *>(usrbuf);

    while (nleft > 0) {
        if ((nread = rio_read_s(rp, ptr, nleft)) < 0) {
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

ssize_t rio_writen_s(rio_t_ssl *rp, void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nwrite;
    char *ptr = static_cast<char *>(usrbuf);

    while (nleft > 0) {
        if ((nwrite = SSL_write(rp->ssl, ptr, nleft)) < 0) {
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

ssize_t rio_read_s(rio_t_ssl *rp, char *usrbuf, size_t n) {
    int cnt;

    while (rp->rio_cnt <= 0) {
        rp->rio_cnt = SSL_read(rp->ssl, rp->rio_buf, sizeof(rp->rio_buf));

        if (rp->rio_cnt < 0) {
            if (errno != EINTR) return -1;
        } else if (rp->rio_cnt == 0) {
            return 0;
        } else {
            rp->rio_bufptr = rp->rio_buf;
        }
    }
    cnt = rp->rio_cnt < n ? rp->rio_cnt : n;
    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -= cnt;
    return cnt;
}

ssize_t rio_readlineb_s(rio_t_ssl *rp, void *usrbuf, size_t maxlen) {
    int n, rc;
    char c;
    char *ptr = static_cast<char *>(usrbuf);
    for (n = 1; n <= maxlen; ++n) {
        if ((rc = rio_read_s(rp, &c, 1)) == 1) {
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
