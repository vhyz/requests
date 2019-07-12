//
// Created by vhyz on 19-5-1.
//

#ifndef REQUESTS_BUFFER_H
#define REQUESTS_BUFFER_H

#include <openssl/ssl.h>
#include <sys/types.h>
#include <functional>

#define RIO_BUFSIZE 8196
#define MAXLINE 4096

class Buffer {
   public:
    using BufferCallBack = std::function<int(char *, size_t)>;

    Buffer();

    void setCallBack(const BufferCallBack &readCallBack,
                     const BufferCallBack &writeCallBack);

    ssize_t readBuffer(char *usrbuf, size_t n);

    ssize_t readLine(void *usrbuf, size_t maxlen);

    ssize_t readNBytes(void *usrbuf, size_t n);

    ssize_t writeNBytes(void *usrbuf, size_t n);

   private:
    int bufCnt;
    char *bufPointer;
    char buf[RIO_BUFSIZE];
    BufferCallBack readCallBack;
    BufferCallBack writeCallBack;
};
#endif  // REQUESTS_BUFFER_H
