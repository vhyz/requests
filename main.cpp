#include <iostream>
#include <string>
#include <vector>
#include "requests.h"
#include <time.h>

int main() {
    RequestOption requestOption;
    auto r = get("http://www.zhihu.com", requestOption);
    std::cout << r->statusCode << std::endl;
    return 0;
}