//
// Created by vhyz on 19-6-29.
//
// decompress gzip data using zlib
//

#include "DecompressGzip.h"
#include <zlib.h>

bool decompressGzip(const std::string& compressedString,
                    std::string& decompressedString) {
    if (compressedString.size() == 0) {
        decompressedString = compressedString;
        return true;
    }
    
    size_t decompressLength = compressedString.size() * 4;

    decompressedString.resize(decompressLength);

    z_stream strm;
    strm.zalloc = nullptr;
    strm.zfree = nullptr;
    strm.opaque = nullptr;

    strm.avail_in = compressedString.size();
    strm.next_in = (Bytef*)compressedString.data();
    strm.total_out = 0;

    if (inflateInit2(&strm, MAX_WBITS + 16) != Z_OK) {
        inflateEnd(&strm);
        return false;
    }

    bool done = false;
    while (!done) {
        // 如果空间不够，扩大一倍空间
        if (strm.total_out >= decompressLength) {
            decompressedString.resize(2 * decompressLength);
            decompressLength *= 2;
        }

        strm.next_out = (Bytef*)(decompressedString.data() + strm.total_out);
        strm.avail_out = decompressLength - strm.total_out;

        int err = inflate(&strm, Z_SYNC_FLUSH);
        if (err == Z_STREAM_END) {
            done = true;
            break;
        }
        if (err != Z_OK) {
            done = false;
            break;
        }
    }

    inflateEnd(&strm);
    if (!done) {
        decompressedString.clear();
        return false;
    }
    decompressedString.resize(strm.total_out);

    return true;
}