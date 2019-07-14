#include <time.h>
#include <iostream>
#include <string>
#include <vector>
#include "requests.h"

int main() {
    requests::RequestOption requestOption;
    requests::Headers headers;
    headers["User-Agent"] =
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like "
        "Gecko) "
        "Chrome/74.0.3729.157 Safari/537.36";
    requestOption.headersPtr = &headers;

    // post data
    rapidjson::Document d;
    d.Parse(R"({"test":"test","hello":"world"})");
    requestOption.jsonPtr = &d;
    requestOption.paramsPtr = &d;
    requestOption.dataPtr = &d;
    requests::HttpResponsePtr r =
        post("http://httpbin.org/post", requestOption);
    std::cout << r->statusCode << std::endl;
    std::cout << r->text << std::endl;

    for (auto &p : r->headers) {
        std::cout << p.first << ": " << p.second << std::endl;
    }

    return 0;
}