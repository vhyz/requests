//
// Created by vhyz on 19-5-1.
//

#ifndef REQUESTS_REQUESTS_H
#define REQUESTS_REQUESTS_H

#include <arpa/inet.h>
#include <iconv.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include "Buffer.h"
#include "Socket.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace requests {
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
    rapidjson::Document *dataPtr;
    rapidjson::Document *paramsPtr;
    rapidjson::Document *jsonPtr;
    Headers* headersPtr;
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

HttpResponsePtr request(const std::string &method, const std::string_view &url,
                        const RequestOption &requestOption);

HttpResponsePtr head(const std::string_view &url,
                     const RequestOption &requestOption);

HttpResponsePtr get(const std::string_view &url,
                    const RequestOption &requestOption);

HttpResponsePtr post(const std::string_view &url,
                     const RequestOption &requestOption);

}  // namespace requests
#endif  // REQUESTS_REQUESTS_H
