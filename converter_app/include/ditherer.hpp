#pragma once

#include <vector>

class Ditherer {
    public:
        bool atkinsonDither(
            std::vector<unsigned char>& data, 
            int width, 
            int height, 
            int stride
        );
};