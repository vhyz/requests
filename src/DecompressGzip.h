//
// Created by vhyz on 19-6-29.
//
// decompress gzip data using zlib
//

#ifndef REQUESTS_DECOMPRESSGZIP_H
#define REQUESTS_DECOMPRESSGZIP_H

#include <stdio.h>
#include <string>

bool decompressGzip(const std::string& compressedString,
                    std::string& decompressedString);

#endif