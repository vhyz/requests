#include <iostream>
#include <string>
#include <vector>
#include "requests.h"
#include <time.h>

int main() {
    RequestOption requestOption;
    HttpResponsePtr r = get("http://www.zhihu.com", requestOption);
    std::cout << r->text << std::endl;
    std::cout << r->statusCode << std::endl;
    return 0;
}