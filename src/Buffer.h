//
// Created by vhyz on 19-5-1.
//

#ifndef REQUESTS_BUFFER_H
#define REQUESTS_BUFFER_H

#include <sys/types.h>

#define RIO_BUFSIZE 8196
#define MAXLINE 4096


class Buffer {
public:
    int fd;
    int bufCnt;
    char *bufPointer;
    char buf[RIO_BUFSIZE];

    Buffer() = default;

    void init(int fd);

    ssize_t readBuffer(char *usrbuf, size_t n);

    ssize_t readLine(void *usrbuf, size_t maxlen);

    ssize_t readNBytes(void *usrbuf, size_t n);

    ssize_t writeNBytes(void *usrbuf, size_t n);
};

#endif //REQUESTS_BUFFER_H
