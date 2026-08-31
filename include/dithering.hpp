#pragma once

#include <vector>

namespace dithering {
    bool atkinsonDither(std::vector<unsigned char>& data, int width, int height, int stride);
}