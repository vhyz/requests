//
// Created by vhyz on 19-5-1.
//

#include "Buffer.h"
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstring>

Buffer::Buffer() {
    this->bufCnt = 0;
    this->bufPointer = buf;
}

void Buffer::setCallBack(const BufferCallBack &readCallBack,
                         const BufferCallBack &writeCallBack) {
    this->readCallBack = readCallBack;
    this->writeCallBack = writeCallBack;
}

ssize_t Buffer::readLine(void *usrbuf, size_t maxlen) {
    int n, rc;
    char c;
    char *ptr = static_cast<char *>(usrbuf);
    for (n = 1; n <= maxlen; ++n) {
        if ((rc = readBuffer(&c, 1)) == 1) {
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

ssize_t Buffer::readBuffer(char *usrbuf, size_t n) {
    int cnt;

    while (bufCnt <= 0) {
        bufCnt = readCallBack(buf, sizeof(buf));
        if (bufCnt < 0) {
            if (errno != EINTR)
                return -1;
        } else if (bufCnt == 0) {
            return 0;
        } else {
            bufPointer = buf;
        }
    }
    cnt = bufCnt < n ? bufCnt : n;
    memcpy(usrbuf, bufPointer, cnt);
    bufPointer += cnt;
    bufCnt -= cnt;
    return cnt;
}

ssize_t Buffer::writeNBytes(void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nwrite;
    char *ptr = static_cast<char *>(usrbuf);

    while (nleft > 0) {
        nwrite = writeCallBack(ptr, nleft);
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

ssize_t Buffer::readNBytes(void *usrbuf, size_t n) {
    size_t nleft = n;
    ssize_t nread;
    char *ptr = static_cast<char *>(usrbuf);

    while (nleft > 0) {
        if ((nread = readBuffer(ptr, nleft)) < 0) {
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