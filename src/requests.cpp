//
// Created by vhyz on 19-5-1.
//

#include "requests.h"
#include <netdb.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <strings.h>
#include <zlib.h>
#include <cstring>
#include <functional>
#include <iostream>
#include <set>
#include <thread>
#include <vector>
#include "DecompressGzip.h"
#include "Logging.h"
#include "Socket.h"
#include "SslSocket.h"

#define CRLF "\r\n"
#define what_is(x) (std::cout << x << std::endl)

void split(std::vector<std::string> &vec, const std::string &s, char c) {
    int start = 0, pos;
    while (start < s.size()) {
        pos = start;
        while (pos < s.size() && s[pos] != c) pos++;
        vec.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
}

std::vector<std::string> split(const std::string &s, char c) {
    std::vector<std::string> vec;
    int start = 0, pos;
    while (start < s.size()) {
        pos = start;
        while (pos < s.size() && s[pos] != c) pos++;
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
    while (pos < s.size() && s[pos] == ' ') pos++;
    s.erase(s.begin(), s.begin() + pos);
}

/*
 *  利用zlib解压缩gzip数据
 */
void decompress(const HttpResponsePtr &response) {
    std::string decompressedString;
    decompressGzip(response->text, decompressedString);
    std::cout << "压缩前 " << response->text.size() << std::endl;
    std::cout << "压缩后 " << decompressedString.size() << std::endl;
    response->text = std::move(decompressedString);
}

/*
 *  已知body长度，读取body
 *  或者Connection为close，未指明长度
 */
void readBodyByContentLength(const std::shared_ptr<HttpResponse> &response,
                             Socket &clientSocket, int contentLength) {
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
void readBodyByChunked(const std::shared_ptr<HttpResponse> &response,
                       Socket &clientSocket) {
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
    static std::set<char> safeCharSet = {'!', '#', '$', '&', '\'', '(', ')',
                                         '*', '+', ',', '/', ':',  ';', '=',
                                         '?', '@', '-', '.', '~'};
    for (const char &c : s) {
        if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
            ('0' <= c && c <= '9')) {
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
    if (line.empty())
        return;
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
    } else if (connectionIterator != response->headers.end() &&
               connectionIterator->second == "close") {
        readBodyByContentLength(response, clientSocket, -1);
    } else if (contentLengthIterator != response->headers.end()) {
        readBodyByContentLength(response, clientSocket,
                                std::stoi(contentLengthIterator->second));
    }
    if (contentEncodingIterator != response->headers.end()) {
        decompress(response);
    }
    clientSocket.shutdownClose();
}

/*
 *  将Json内容转化为Url-Encode的data
 */
void convertJsonToUrlEncodeData(std::string &str,
                                const rapidjson::Document &document) {
    if (document.IsObject()) {
        for (rapidjson::Value::ConstMemberIterator iterator =
                 document.MemberBegin();
             iterator != document.MemberEnd(); ++iterator) {
            assert(iterator->value.IsString() || iterator->value.IsArray());

            if (iterator->value.IsString()) {
                str += urlEncode(iterator->name.GetString());
                str.push_back('=');
                str += urlEncode(iterator->value.GetString());
            } else {
                for (rapidjson::SizeType i = 0; i < iterator->value.Size();
                     ++i) {
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
std::string_view parseUrl(const std::string_view &url, std::string &sendMsg,
                          int pos, rapidjson::Document *document) {
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

    if (document != nullptr){
        sendMsg.push_back('?');
        convertJsonToUrlEncodeData(sendMsg, *document);
    }

    sendMsg += " HTTP/1.1";
    sendMsg += CRLF;
    return url.substr(start, pos - start);
}

/*
 *  暴露在外的request接口
 */
HttpResponsePtr request(const std::string &method, const std::string_view &url,
                        const RequestOption &requestOption) {
    Dict sendHeader = {{"User-Agent", "C++-requests"},
                       {"Accept-Encoding", "gzip"},
                       {"Accept", "*/*"},
                       {"Connection", "keep-alive"}};
    std::string sendMsg = method;
    for (auto &p : requestOption.headers) {
        sendHeader[p.first] = p.second;
    }
    int port;
    bool isHttps;
    std::string host, body;
    if (url.substr(0, 5) == "http:") {
        port = 80;
        isHttps = false;
        host = sendHeader["Host"] =
            parseUrl(url, sendMsg, 7, requestOption.params);
    } else {
        port = 443;
        isHttps = true;
        host = sendHeader["Host"] =
            parseUrl(url, sendMsg, 8, requestOption.params);
    }
    if (requestOption.data != nullptr && requestOption.data->IsObject()) {
        convertJsonToUrlEncodeData(body, *requestOption.data);
        sendHeader["Content-Type"] = "application/x-www-form-urlencoded";
        sendHeader["Content-Length"] = std::to_string(body.size());
    }
    if (requestOption.json != nullptr && !requestOption.json->IsNull()) {
        rapidjson::StringBuffer stringBuffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(stringBuffer);
        requestOption.json->Accept(writer);
        body = stringBuffer.GetString();
        sendHeader["Content-Type"] = "application/json";
        sendHeader["Content-Length"] = std::to_string(body.size());
    }
    for (auto &p : sendHeader) {
        sendMsg += p.first + ": " + p.second + CRLF;
    }
    sendMsg += CRLF;
    sendMsg += body;
    what_is(sendMsg);
    auto hostPtr = gethostbyname(host.c_str());
    HttpResponsePtr response = std::make_shared<HttpResponse>();
    if (hostPtr != nullptr) {
        auto pptr = hostPtr->h_addr_list;
        const char *ip = inet_ntoa(*((in_addr *)*pptr));
        if (isHttps) {
            SslSocket clientSocket(ip, port);
            if (clientSocket.connect() == false) {
                return response;
            }
            clientSocket.send(sendMsg);
            request_help(clientSocket, response);
        } else {
            Socket clientSocket(ip, port);
            if (clientSocket.connect() == false) {
                return response;
            }
            clientSocket.send(sendMsg);
            request_help(clientSocket, response);
        }
        if (response->statusCode == 301) {
            return request(method, response->headers["Location"],
                           requestOption);
        }
    }
    return response;
}

HttpResponsePtr head(const std::string_view &url,
                     const RequestOption &requestOption) {
    return request("HEAD", url, requestOption);
}

HttpResponsePtr get(const std::string_view &url,
                    const RequestOption &requestOption) {
    return request("GET", url, requestOption);
}

HttpResponsePtr post(const std::string_view &url,
                     const RequestOption &requestOption) {
    return request("POST", url, requestOption);
}

RequestOption::RequestOption() {
    data = nullptr;
    params = nullptr;
    json = nullptr;
    timeout = -1;
}

CharsetConverter::CharsetConverter(const char *fromCharset,
                                   const char *toCharset) {
    cd = iconv_open(toCharset, fromCharset);
}

CharsetConverter::~CharsetConverter() { iconv_close(cd); }

int CharsetConverter::convert(const char *inbuf, int inlen, char *outbuf,
                              int outlen) {
    memset(outbuf, 0, outlen);
    return iconv(cd, (char **)&inbuf, (size_t *)&inlen, &outbuf,
                 (size_t *)&outlen);
}
