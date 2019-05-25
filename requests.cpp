//
// Created by vhyz on 19-5-1.
//

#include <strings.h>
#include <iostream>
#include <functional>
#include <netdb.h>
#include "requests.h"
#include <vector>
#include <thread>
#include <zlib.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <cstring>
#include <set>
#include "Socket.h"
#include "SslClientSocket.h"
#include "ClientSokcet.h"
#include "Logging.h"

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
 *  或者Connection为close，未指明长度
 */
void
readBodyByContentLength(const std::shared_ptr<HttpResponse> &response, Socket &clientSocket, int contentLength) {
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
void readBodyByChunked(const std::shared_ptr<HttpResponse> &response, Socket &clientSocket) {

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
std::string urlEncode(const std::string_view &s) {
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

void request_help(Socket &clientSocket, const HttpResponsePtr &response) {
    std::string line = clientSocket.readLine();
    std::string line1 = clientSocket.readLine();
    std::vector<std::string> vec;
    split(vec, line, ' ');
    response->statusCode = std::stoi(vec[1]);
    while (true) {
        line = clientSocket.readLine();
        what_is(line);
        if (line.empty() || line == "\r")
            break;
        int pos = line.find(':');
        std::string headersKey = line.substr(0, pos);
        std::string headersValue = line.substr(pos + 1, line.size() - pos - 1);
        updateStandardHeadersField(headersKey);
        deleteHeadersValueSpace(headersValue);
        response->headers[std::move(headersKey)] = std::move(headersValue);
    }
    auto contentLengthIterator = response->headers.find("Content-Length");
    auto transferEncodingIterator = response->headers.find("Transfer-Encoding");
    auto contentEncodingIterator = response->headers.find("Content-Encoding");
    auto connectionIterator = response->headers.find("Connection");
    if (transferEncodingIterator != response->headers.end()) {
        readBodyByChunked(response, clientSocket);
    } else if (connectionIterator != response->headers.end() && connectionIterator->second == "close") {
        readBodyByContentLength(response, clientSocket, -1);
    } else if (contentLengthIterator != response->headers.end()) {
        readBodyByContentLength(response, clientSocket, std::stoi(contentLengthIterator->second));
    }
    clientSocket.shutdownClose();
}

/*
 *  将Json内容转化为Url-Encode的data
 */
void convertJsonToUrlEncodeData(std::string &str, const rapidjson::Document &document) {

    if (document.IsObject()) {
        str.push_back('?');
        for (rapidjson::Value::ConstMemberIterator iterator = document.MemberBegin();
             iterator != document.MemberEnd(); ++iterator) {

            assert(iterator->value.IsString() || iterator->value.IsArray());

            if (iterator->value.IsString()) {
                str += urlEncode(iterator->name.GetString());
                str.push_back('=');
                str += urlEncode(iterator->value.GetString());
            } else {
                for (rapidjson::SizeType i = 0; i < iterator->value.Size(); ++i) {

                    assert(iterator->value[i].IsString());

                    str += urlEncode(iterator->name.GetString());
                    str.push_back('=');
                    str += urlEncode(iterator->value[i].GetString());

                    if (i + 1 != iterator->value.Size())
                        str.push_back('&');
                }
            }
            if (iterator + 1 != document.MemberEnd()) {
                str.push_back('&');
            }
        }
    }
}

/*
 *  解析url，返回host
 *  为了防止拷贝，返回string_view
 *  这是因为url的生命周期不会过早结束
 */
std::string_view
parseUrl(const std::string_view &url, std::string &sendMsg, int pos, const rapidjson::Document &document) {
    int start = pos;
    for (; pos < url.size(); ++pos) {
        if (url[pos] == '/')
            break;
    }
    if (pos == url.size()) {
        sendMsg += " /";
    } else {
        sendMsg += " " + urlEncode(url.substr(pos, url.size() - pos));
    }

    convertJsonToUrlEncodeData(sendMsg, document);

    sendMsg += " HTTP/1.1";
    sendMsg += CRLF;
    return url.substr(start, pos - start);
}

/*
 *  暴露在外的request接口
 */
HttpResponsePtr request(const std::string &method, const std::string_view &url, const RequestOption &requestOption) {

    Dict sendHeader = {
            {"User-Agent",      "C++-requests"},
            {"Accept-Encoding", "gzip, deflate"},
            {"Accept",          "*/*"},
            {"Connection",      "keep-alive"}
    };

    std::string sendMsg = method;
    for (auto &p:requestOption.headers) {
        sendHeader[p.first] = p.second;
    }
    int port;
    bool isHttps;
    std::string host, body;
    if (url.substr(0, 5) == "http:") {
        port = 80;
        isHttps = false;
        host = sendHeader["Host"] = parseUrl(url, sendMsg, 7, requestOption.params);
    } else {
        port = 443;
        isHttps = true;
        host = sendHeader["Host"] = parseUrl(url, sendMsg, 8, requestOption.params);
    }
    if (requestOption.data.IsObject()) {
        convertJsonToUrlEncodeData(body, requestOption.data);
        sendHeader["Content-Type"] = "application/x-www-form-urlencoded";
    }
    if (!requestOption.json.IsNull()) {
        rapidjson::StringBuffer stringBuffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(stringBuffer);
        requestOption.json.Accept(writer);
        sendMsg += stringBuffer.GetString();
        sendHeader["Content-Type"] = "application/json";
    }
    for (auto &p :sendHeader) {
        sendMsg += p.first + ": " + p.second + CRLF;
    }
    sendMsg += CRLF;
    sendMsg += body;
    what_is(sendMsg);
    auto hostPtr = gethostbyname(host.c_str());
    HttpResponsePtr response = std::make_shared<HttpResponse>();
    for (auto pptr = hostPtr->h_addr_list; *pptr != nullptr; ++pptr) {
        const char *ip = inet_ntoa(*((in_addr *) *pptr));
        if (isHttps) {
            SslClientSocket clientSocket(ip, port);
            clientSocket.send(sendMsg);
            request_help(clientSocket, response);
        } else {
            ClientSocket clientSocket(ip, port);
            clientSocket.send(sendMsg);
            request_help(clientSocket, response);
        }
        break;
    }
    if (response->statusCode == 301) {
        return request(method, response->headers["Location"], requestOption);
    }
    return response;
}

HttpResponsePtr head(const std::string_view &url, const RequestOption &requestOption) {
    return request("HEAD", url, requestOption);
}

HttpResponsePtr get(const std::string_view &url, const RequestOption &requestOption) {
    return request("GET", url, requestOption);
}

HttpResponsePtr post(const std::string_view &url, const RequestOption &requestOption) {
    return request("POST", url, requestOption);
}

HttpResponsePtr put(const std::string_view &url, const RequestOption &requestOption) {
    return request("PUT", url, requestOption);
}

HttpResponsePtr patch(const std::string_view &url, const RequestOption &requestOption) {
    return request("PATCH", url, requestOption);
}

HttpResponsePtr Delete(const std::string_view &url, const RequestOption &requestOption) {
    return request("DELETE", url, requestOption);
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
