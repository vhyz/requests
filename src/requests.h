//
// Created by vhyz on 19-5-1.
//

#ifndef REQUESTS_REQUESTS_H
#define REQUESTS_REQUESTS_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <string_view>
#include <string>
#include <unistd.h>
#include <map>
#include <memory>
#include <iconv.h>
#include <ostream>
#include "Buffer.h"
#include "Socket.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

using Dict = std::map<std::string, std::string>;
using Headers = Dict;

class HttpResponse {
public:
    Headers headers;
    std::string text;
    int statusCode;
};

class RequestOption {
public:
    rapidjson::Document* data;
    rapidjson::Document* params;
    rapidjson::Document* json;
    Dict headers;
    int timeout;
    RequestOption();
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

HttpResponsePtr request(const std::string &method, const std::string_view &url, const RequestOption &requestOption);

HttpResponsePtr head(const std::string_view &url, const RequestOption &requestOption);

HttpResponsePtr get(const std::string_view &url, const RequestOption &requestOption);

HttpResponsePtr post(const std::string_view &url, const RequestOption &requestOption);


#endif //REQUESTS_REQUESTS_H
