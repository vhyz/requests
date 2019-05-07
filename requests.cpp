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
#include <cstring>
#include <set>

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
    buffer.writeNBytes((void *) msg, n);
}

int Socket::recv(char *p, ssize_t n) {
    return buffer.readBuffer(p, n);
}

std::string Socket::recv() {
    char buf[MAXLINE];
    int n = buffer.readBuffer(buf, MAXLINE);
    return std::string(buf, n);
}

Socket::Socket(const std::string &addr, int port) {
    sockfd_init(addr.c_str(), port);
}

Socket::Socket(const char *addr, int port) {
    sockfd_init(addr, port);
    buffer.init(sockfd);
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
        int n = buffer.readLine(buf, MAXLINE);
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
    buffer.readNBytes(buf, n);
    std::string s(buf, n);
    delete[] buf;
    return s;
}

int Socket::readNBytes(char *p, size_t n) {
    return buffer.readNBytes(p, n);
}

ClientSocket::ClientSocket(const char *addr, int port) : Socket(addr, port) {
    connect(sockfd, (sockaddr *) &sockaddrIn, sizeof sockaddrIn);
}

ClientSocket::ClientSocket(const std::string &addr, int port) : ClientSocket(addr.c_str(), port) {}

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

    int err = inflateInit2(&strm, MAX_WBITS + 16);
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

/*
 *  已知body长度，读取body
 */
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

/*
 *  以 chunked 的方式分段读取数据body
 */
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
    char buf[1024 * 1024];
    int n = decompress(response->text.c_str(), response->text.size(), buf, sizeof buf);
    what_is(n);
    response->text = std::string(buf, n);
}

/*
 *  十进制转十六进制
 */
char dec2hexChar(int n) {
    if (0 <= n && n <= 9)
        return '0' + n;
    else
        return 'A' + n - 10;
}

/*
 *  网址编码函数
 */
std::string urlEncode(const std::string &s) {
    std::string res;
    static std::set<char> safeCharSet = {'!', '#', '$', '&', '\'', '(', ')', '*', '+', ',', '/', ':', ';', '=', '?',
                                         '@', '-', '.', '~'};
    for (const char &c:s) {
        if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9')) {
            res.push_back(c);
            continue;
        }
        auto it = safeCharSet.find(c);
        if (it != safeCharSet.end()) {
            res.push_back(c);
        } else {
            res.push_back('%');
            int j = c;
            if (j < 0) {
                j += 256;
            }
            int a = j / 16;
            int b = j - a * 16;
            res.push_back(dec2hexChar(a));
            res.push_back(dec2hexChar(b));
        }
    }
    return res;
}

/*
 *  暴露在外的request接口
 */
std::shared_ptr<HttpResponse>
request(const std::string &method, const std::string &url, const RequestOption &requestOption) {
    std::string sendMsg = method;
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
            sendMsg += " " + urlEncode(url.substr(pos, url.size() - pos));
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
            sendMsg += " " + urlEncode(url.substr(pos, url.size() - pos));
        }
        sendMsg += " HTTP/1.1";
        sendMsg += CRLF;
        host = sendHeader["Host"] = url.substr(8, pos - 8);
    }
    for (auto &p :sendHeader) {
        sendMsg += p.first + ": " + p.second + CRLF;
    }
    sendMsg += CRLF;
    what_is(sendMsg);
    auto hostPtr = gethostbyname(host.c_str());
    std::shared_ptr<HttpResponse> response = std::make_shared<HttpResponse>();
    for (auto pptr = hostPtr->h_addr_list; *pptr != nullptr; ++pptr) {
        const char *ip = inet_ntoa(*((in_addr *) *pptr));
        SslClientSocket clientSocket(ip, port);
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
        CharsetConverter charsetConverter("GBK", "UTF-8");
        char buf[512000];
        int n = charsetConverter.convert(response->text.c_str(), response->text.size(), buf, sizeof buf);
        if (n == -1) {
            what_is(strerror(errno));
            return response;
        }
        what_is(n);
        response->text = std::string(buf, n);
        what_is(response->text);
        clientSocket.shutdownClose();
        break;
    }
    return response;
}

std::shared_ptr<HttpResponse> head(const std::string &url, const RequestOption &requestOption) {
    return request("HEAD", url, requestOption);
}

std::shared_ptr<HttpResponse> get(const std::string &url, const RequestOption &requestOption) {
    return request("GET", url, requestOption);
}

std::shared_ptr<HttpResponse> post(const std::string &url, const RequestOption &requestOption) {
    return request("POST", url, requestOption);
}

std::shared_ptr<HttpResponse> put(const std::string &url, const RequestOption &requestOption) {
    return request("PUT", url, requestOption);
}

std::shared_ptr<HttpResponse> patch(const std::string &url, const RequestOption &requestOption) {
    return request("PATCH", url, requestOption);
}

std::shared_ptr<HttpResponse> Delete(const std::string &url, const RequestOption &requestOption) {
    return request("Delete", url, requestOption);
}

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
        ctx = SSL_CTX_new(TLS_client_method());
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

CharsetConverter::CharsetConverter(const char *fromCharset, const char *toCharset) {
    cd = iconv_open(toCharset, fromCharset);
}

CharsetConverter::~CharsetConverter() {
    iconv_close(cd);
}

int CharsetConverter::convert(const char *inbuf, int inlen, char *outbuf, int outlen) {
    memset(outbuf, 0, outlen);
    return iconv(cd, (char **) &inbuf, (size_t *) &inlen, &outbuf, (size_t *) &outlen);
}
