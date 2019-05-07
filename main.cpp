#include <iostream>
#include <string>
#include <vector>
#include "requests.h"
#include <time.h>

int main() {
    RequestOption requestOption;
    auto r = get("https://translate.google.cn/", requestOption);
    return 0;
}