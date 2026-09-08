#pragma once

#include <string>

class BitPacker {
    public:
        bool packCStyleArray(int width, int height, int stride, const unsigned char* data, std::string filename, std::string arrayname);
        bool packBinaryFile(int width, int height, int stride, const unsigned char* data, std::string filename);
};