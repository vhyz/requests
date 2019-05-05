#include <iostream>
#include <string>
#include <vector>
#include "requests.h"
#include <time.h>

int main() {
    RequestOption requestOption;
    auto r = get("https://vhyz.me/", requestOption);
    return 0;
}