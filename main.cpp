#include <iostream>
#include <string>
#include <vector>
#include "requests.h"
#include <time.h>

int main() {
    RequestOption requestOption;
    for (int i = 0; i < 10; ++i) {
        auto r = get("http://www.cnblogs.com/vhyz", requestOption);
    }
    return 0;
}