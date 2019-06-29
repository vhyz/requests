//
// Created by vhyz on 19-6-29.
//
// decompress gzip data using zlib
//

#include <stdio.h>
#include <string>

bool decompressGzip(const std::string& compressedString,
                    std::string& decompressedString);