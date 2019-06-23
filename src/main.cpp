#include <time.h>
#include <iostream>
#include <string>
#include <vector>
#include "requests.h"

int main() {
    RequestOption requestOption;
    requestOption.headers["User-Agent"] =
        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like "
        "Gecko) "
        "Chrome/74.0.3729.157 Safari/537.36";
    std::string url;
    getline(std::cin, url);
    HttpResponsePtr r = get(url, requestOption);
    // std::cout << r->text << std::endl;
    std::cout << r->statusCode << std::endl;
    return 0;
}