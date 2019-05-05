//
// Created by vhyz on 19-5-1.
//

#include <strings.h>
#include <iostream>
#include <functional>
#include <netdb.h>
#include "requests.h"
#include <vector>
#include <zlib.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define CRLF "\r\n"
#define what_is(x) (std::cout<<x<<std::endl)

void split(std::vector<std::string> &vec, const std::string &s, char c) {
    int start = 0, pos;
    while (start < s.size()) {
        pos = start;
        while (pos < s.size() && s[pos] != c)
            pos++;
        vec.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
}

std::vector<std::string> split(const std::string &s, char c) {
    std::vector<std::string> vec;
    int start = 0, pos;
    while (start < s.size()) {
        pos = start;
        while (pos < s.size() && s[pos] != c)
            pos++;
        vec.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
    return vec;
}

void updateStandardHeadersField(std::string &field) {
    if (field == "ETag")
        return;
    for (int i = 0; i < field.size(); ++i) {
        if (islower(field[i])) {
            field[i] -= 32;
        }
        i++;
        while (i < field.size() && field[i] != '-') {
            if (isupper(field[i]))
                field[i] += 32;
            i++;
        }
    }
}

void deleteHeadersValueSpace(std::string &s) {
    int pos = 0;
    while (pos < s.size() && s[pos] == ' ')
        pos++;
    s.erase(s.begin(), s.begin() + pos);
}

void Socket::send(const std::string &s) {
    send(s.c_str(), s.size());
}

void Socket::send(const char *msg, ssize_t n) {
    rio.rioWriten((void *) msg, n);
}

int Socket::revc(char *p, ssize_t n) {
    return rio.rioRead(p, n);
}

std::string Socket::recv() {
    char buf[MAXLINE];
    int n = rio.rioRead(buf, MAXLINE);
    return std::string(buf, n);
}

Socket::Socket(const std::string &addr, int port) {
    sockfd_init(addr.c_str(), port);
}

Socket::Socket(const char *addr, int port) {
    sockfd_init(addr, port);
    rio.init(sockfd, false);
}

void Socket::shutdownClose() {
    close(sockfd);
}

void Socket::sockfd_init(const char *addr, int port) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&sockaddrIn, sizeof sockaddrIn);
    sockaddrIn.sin_family = AF_INET;
    sockaddrIn.sin_port = htons(port);
    inet_pton(AF_INET, addr, &sockaddrIn.sin_addr);
}

std::string Socket::readLine() {
    std::string res;
    char buf[MAXLINE];
    while (true) {
        int n = rio.rioReadLine(buf, MAXLINE);
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

std::string Socket::readNBytes(int n) {
    char *buf = new char[n];
    rio.rioReadNBytes(buf, n);
    std::string s(buf, n);
    delete[] buf;
    return s;
}

HttpClientSocket::HttpClientSocket(const char *addr, int port) : Socket(addr, port) {
    connect(sockfd, (sockaddr *) &sockaddrIn, sizeof sockaddrIn);
}

HttpClientSocket::HttpClientSocket(const std::string &addr, int port) : HttpClientSocket(addr.c_str(), port) {}

std::map<std::string, std::function<std::shared_ptr<HttpResponse>(const std::string &,
                                                                  const RequestOption &requestOption)>> requestDict = {
        {"head",  head},
        {"get",   get},
        {"post",  post},
        {"put",   put},
        {"patch", patch},
};

const Dict baseHeader = {
        {"User-Agent",      "C++-requests"},
        {"Accept-Encoding", "gzip, deflate"},
        {"Accept",          "*/*"},
        {"Connection",      "keep-alive"}
};

int decompress(const char *src, int srcLen, char *dst, int dstLen) {
    z_stream strm;
    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.opaque = nullptr;

    strm.avail_in = srcLen;
    strm.avail_out = dstLen;
    strm.next_in = (Bytef *) src;
    strm.next_out = (Bytef *) dst;

    int err = -1;
    err = inflateInit2(&strm, MAX_WBITS + 16);
    //err = inflateInit(&strm);
    if (err == Z_OK) {
        err = inflate(&strm, Z_FINISH);
        if (err == Z_STREAM_END) {
            (void) inflateEnd(&strm);
            return strm.total_out;
        } else {
            (void) inflateEnd(&strm);
            return -1;
        }
    } else {
        inflateEnd(&strm);
        return 0;
    }
}

void
readBodyByContentLength(std::shared_ptr<HttpResponse> &response, Socket &clientSocket, int contentLength) {
    while (true) {
        std::string text = clientSocket.recv();
        response->text += text;
        if (text.empty())
            break;
        if (response->text.size() == contentLength)
            break;
    }
}

void readBodyByChunked(std::shared_ptr<HttpResponse> &response, Socket &clientSocket) {
    while (true) {
        std::string text = clientSocket.readLine();
        int length = std::stoi(text, nullptr, 16);
        if (length == 0) {
            clientSocket.readLine();
            break;
        }
        response->text += clientSocket.readNBytes(length);
        clientSocket.readLine();
    }
    char buf[100000];
    int n = decompress(response->text.c_str(), response->text.size(), buf, sizeof buf);
    response->text = std::string(buf, n);
}

std::shared_ptr<HttpResponse>
request(const std::string &method, const std::string &url, const RequestOption &requestOption) {
    return requestDict[method](url, requestOption);
}

std::shared_ptr<HttpResponse> head(const std::string &url, const RequestOption &requestOption) {
}

std::shared_ptr<HttpResponse> get(const std::string &url, const RequestOption &requestOption) {
    std::string sendMsg = "GET";
    Dict sendHeader = baseHeader;
    sendHeader.insert(requestOption.headers.begin(), requestOption.headers.end());
    std::string host;
    int port;
    if (url.substr(0, 5) == "http:") {
        port = 80;
        int pos;
        for (pos = 7; pos < url.size(); ++pos) {
            if (url[pos] == '/')
                break;
        }
        if (pos == url.size()) {
            sendMsg += " /";
        } else {
            sendMsg += " " + url.substr(pos, url.size() - pos);
        }
        sendMsg += " HTTP/1.1";
        sendMsg += CRLF;
        host = sendHeader["Host"] = url.substr(7, pos - 7);
    } else {
        port = 443;
        int pos;
        for (pos = 8; pos < url.size(); ++pos) {
            if (url[pos] == '/')
                break;
        }
        if (pos == url.size()) {
            sendMsg += " /";
        } else {
            sendMsg += " " + url.substr(pos, url.size() - pos);
        }
        sendMsg += " HTTP/1.1";
        sendMsg += CRLF;
        host = sendHeader["Host"] = url.substr(8, pos - 8);
    }
    for (auto &p :sendHeader) {
        sendMsg += p.first + ": " + p.second + CRLF;
    }
    sendMsg += CRLF;
    auto hostPtr = gethostbyname(host.c_str());
    std::shared_ptr<HttpResponse> response = std::make_shared<HttpResponse>();
    for (auto pptr = hostPtr->h_addr_list; *pptr != nullptr; ++pptr) {
        const char *ip = inet_ntoa(*((in_addr *) *pptr));
        HttpsClientSocket clientSocket(ip, port);
        clientSocket.send(sendMsg);
        std::string line = clientSocket.readLine();
        std::vector<std::string> vec;
        split(vec, line, ' ');
        response->statusCode = std::stoi(vec[1]);
        while (true) {
            line = clientSocket.readLine();
            what_is(line);
            if (line.empty() || line == "\r")
                break;
            auto v = split(line, ':');
            updateStandardHeadersField(v[0]);
            deleteHeadersValueSpace(v[1]);
            response->headers[std::move(v[0])] = std::move(v[1]);
        }
        auto contentLengthIterator = response->headers.find("Content-Length");
        auto transferEncodingIterator = response->headers.find("Transfer-Encoding");
        if (contentLengthIterator != response->headers.end()) {
            readBodyByContentLength(response, clientSocket, std::stoi(contentLengthIterator->second));
        }
        if (transferEncodingIterator != response->headers.end()) {
            readBodyByChunked(response, clientSocket);
        }
        what_is(response->text);
        break;
    }
    return response;
}

std::shared_ptr<HttpResponse> post(const std::string &url, const RequestOption &requestOption) {

}

std::shared_ptr<HttpResponse> put(const std::string &url, const RequestOption &requestOption) {

}

std::shared_ptr<HttpResponse> patch(const std::string &url, const RequestOption &requestOption) {

}

HttpsClientSocket::HttpsClientSocket(const char *addr, int port) : Socket(addr, port) {
    sockfd_init(addr, port);
    rio.init(sockfd, true);
    SSL_load_error_strings();
    SSLeay_add_ssl_algorithms();
    const SSL_METHOD *method = SSLv23_client_method();
    rio.ctx = SSL_CTX_new(method);
    connect(sockfd, (sockaddr *) &sockaddrIn, sizeof(sockaddrIn));
    rio.ssl = SSL_new(rio.ctx);
    SSL_set_fd(rio.ssl, sockfd);
    SSL_connect(rio.ssl);
}

HttpsClientSocket::HttpsClientSocket(const std::string &addr, int port) : HttpsClientSocket(addr.c_str(), port) {}

void HttpsClientSocket::shutdownClose() {
    SSL_shutdown(rio.ssl);
    SSL_free(rio.ssl);
    SSL_CTX_free(rio.ctx);
    Socket::shutdownClose();
}