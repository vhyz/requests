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
#include "rio.h"

#define MAXLINE 4096

using Dict = std::map<std::string, std::string>;
using Headers = Dict;

/*
 * Socket : base class of ClientSocket and ServerSocket
 */
class Socket {
protected:
    int sockfd;
    rio_t rio;
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

    std::string recv();

    std::string readNBytes(int n);

    int revc(char *p, ssize_t n);

    std::string readLine();
};

class HttpClientSocket : public Socket {
public:
    HttpClientSocket(const char *addr, int port);

    HttpClientSocket(const std::string &addr, int port);
};

class HttpsClientSocket : public Socket {
public:
    HttpClientSocket(const char*addr,int port);

    HttpClientSocket(const std::string &addr,int port);
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
};

std::pair<Headers, int> readHeaders(const std::string &s);

std::shared_ptr<HttpResponse>
request(const std::string &method, const std::string &url, const RequestOption &requestOption);

std::shared_ptr<HttpResponse> head(const std::string &url, const RequestOption &requestOption);

std::shared_ptr<HttpResponse> get(const std::string &url, const RequestOption &requestOption);

std::shared_ptr<HttpResponse> post(const std::string &url, const RequestOption &requestOption);

std::shared_ptr<HttpResponse> put(const std::string &url, const RequestOption &requestOption);

std::shared_ptr<HttpResponse> patch(const std::string &url, const RequestOption &requestOption);


#endif //REQUESTS_REQUESTS_H
