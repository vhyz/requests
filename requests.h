//
// Created by vhyz on 19-5-1.
//

#ifndef REQUESTS_REQUESTS_H
#define REQUESTS_REQUESTS_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string>
#include <unistd.h>
#include <map>
#include <memory>
#include <iconv.h>
#include <ostream>
#include "Buffer.h"

#define MAXLINE 4096

using Dict = std::map<std::string, std::string>;
using Headers = Dict;

/*
 * Socket : base class of ClientSocket and ServerSocket
 */
class Socket {
protected:
    int sockfd;
    Buffer buffer;
    sockaddr_in sockaddrIn;

    void sockfd_init(const char *addr, int port);

public:
    Socket(const char *addr, int port);

    Socket(const std::string &addr, int port);

/*
 *
 */
    virtual void shutdownClose();

    virtual void send(const std::string &s);

    virtual void send(const char *msg, ssize_t n);

    virtual std::string recv();

    virtual int recv(char *p, ssize_t n);

    virtual std::string readNBytes(int n);

    virtual std::string readLine();

    virtual int readNBytes(char *p, size_t n);
};

class ClientSocket : public Socket {
public:
    ClientSocket(const char *addr, int port);

    ClientSocket(const std::string &addr, int port);

};

class SslClientSocket : public Socket {
public:
    SslClientSocket(const char *addr, int port);

    SslClientSocket(const std::string &addr, int port);

    void shutdownClose() override;

    inline void send(const std::string &s) override;

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

class ServerSocket : public Socket {
public:

};


class HttpResponse {

public:
    Headers headers;
    std::string text;
    int statusCode;
};

class RequestOption {
public:
    Dict params;
    Dict data;
    Dict headers;
    int timeout;

    RequestOption() = default;
};

class CharsetConverter {
private:
    iconv_t cd;
public:
    CharsetConverter(const char *fromCharset, const char *toCharset);

    ~CharsetConverter();

    int convert(const char *inbuf, int inlen, char *outbuf, int outlen);
};

using HttpResponsePtr = std::shared_ptr<HttpResponse>;

HttpResponsePtr request(const std::string &method, const std::string &url, const RequestOption &requestOption);

HttpResponsePtr head(const std::string &url, const RequestOption &requestOption);

HttpResponsePtr get(const std::string &url, const RequestOption &requestOption);

HttpResponsePtr post(const std::string &url, const RequestOption &requestOption);

HttpResponsePtr put(const std::string &url, const RequestOption &requestOption);

HttpResponsePtr patch(const std::string &url, const RequestOption &requestOption);


#endif //REQUESTS_REQUESTS_H
