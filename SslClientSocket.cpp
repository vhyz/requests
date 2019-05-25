//
// Created by vhyz on 19-5-12.
//
#include "SslClientSocket.h"
#include <cstring>

SslClientSocket::SslClientSocket(const char *addr, int port) : Socket(addr, port) {
    sockfd_init(addr, port);
    buffer.init(sockfd);
    sslInit();
    createCtx();
    connect(sockfd, (sockaddr *) &sockaddrIn, sizeof(sockaddrIn));
    ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sockfd);
    SSL_connect(ssl);
}

SslClientSocket::SslClientSocket(const std::string &addr, int port) : SslClientSocket(addr.c_str(), port) {}

void SslClientSocket::shutdownClose() {
    SSL_shutdown(ssl);
    Socket::shutdownClose();
}

SSL_CTX *SslClientSocket::ctx = nullptr;
bool SslClientSocket::isSslInit = false;

void SslClientSocket::sslInit() {
    if (!isSslInit) {
        SSL_library_init();
        SSL_load_error_strings();
    }
}

void SslClientSocket::freeCtx() {
    if (ctx != nullptr) {
        SSL_CTX_free(ctx);
        ctx = nullptr;
    }
}

void SslClientSocket::createCtx() {
    if (ctx == nullptr) {
        ctx = SSL_CTX_new(TLSv1_client_method());
    }
}

void SslClientSocket::send(const std::string &msg) {
    send(msg.c_str(), msg.size());
}

void SslClientSocket::send(const char *msg, ssize_t n) {
    sslWriteNBytes((void *) msg, n);
}

std::string SslClientSocket::readNBytes(int n) {
    char *buf = new char[n];
    sslReadNBytes(buf, n);
    std::string s(buf, n);
    delete[] buf;
    return s;
}

std::string SslClientSocket::readLine() {
    std::string res;
    char buf[MAXLINE];
    while (true) {
        int n = sslReadLine(buf, MAXLINE);
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

int SslClientSocket::recv(char *p, ssize_t n) {
    return sslReadBuffer(p, n);
}

std::string SslClientSocket::recv() {
    char buf[MAXLINE];
    int n = recv(buf, MAXLINE);
    return std::string(buf, n);
}

ssize_t SslClientSocket::sslReadBuffer(char *usrbuf, size_t n) {

    while (buffer.bufCnt <= 0) {
        buffer.bufCnt = SSL_read(ssl, buffer.buf, sizeof(buffer.buf));

        if (buffer.bufCnt > 0) {
            buffer.bufPointer = buffer.buf;
        } else if (buffer.bufCnt == 0) {
            return 0;
        } else {
            if (errno != EINTR)
                return -1;
        }
    }

    int cnt = buffer.bufCnt < n ? buffer.bufCnt : n;
    memcpy(usrbuf, buffer.bufPointer, cnt);
    buffer.bufCnt -= cnt;
    buffer.bufPointer += cnt;
    return cnt;
}

ssize_t SslClientSocket::sslReadLine(void *usrbuf, size_t maxlen) {
    int n, rc;
    char c;
    char *ptr = static_cast<char *>(usrbuf);
    for (n = 1; n <= maxlen; ++n) {
        if ((rc = sslReadBuffer(&c, 1)) == 1) {
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

ssize_t SslClientSocket::sslReadNBytes(void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char *ptr = static_cast<char *>(usrbuf);

    while (nleft > 0) {
        if ((nread = sslReadBuffer(ptr, nleft)) < 0) {
            return nread;
        } else if (nread == 0)
            break;
        nleft -= nread;
        ptr += nread;
    }
    return n - nleft;
}

ssize_t SslClientSocket::sslWriteNBytes(void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nwrite;
    char *ptr = static_cast<char *>(usrbuf);

    while (nleft > 0) {
        nwrite = SSL_write(ssl, ptr, nleft);
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

int SslClientSocket::readNBytes(char *p, size_t n) {
    return sslReadNBytes(p, n);
}