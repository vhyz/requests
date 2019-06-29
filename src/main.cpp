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
    std::string urls[] = {"https://www.baidu.com",
                          "https://www.github.com",
                          "https://www.zhihu.com",
                          "https://vhyz.github.io",
                          "http://hssgame.com",
                          "https://club.huawei.com/forum-2404-3.html",
                          "https://www.bilibili.com/video/av24353925/",
                          "https://www.sina.com.cn/"};
    for (size_t i = 0; i < sizeof(urls) / sizeof(std::string); ++i) {
        HttpResponsePtr r = get(urls[i], requestOption);
        // std::cout << r->text << std::endl;
        std::cout << r->statusCode << std::endl;
        getchar();
    }
    return 0;
}