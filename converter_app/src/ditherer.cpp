#include "ditherer.hpp"
#include <algorithm>


// Takes a vector (1d) of copied image data. Basically an array of bytes for each pixel. 
// height, width, and stride (bytes per row).
// Returns true upon success, modifies the image data in place to apply Atkinson Dithering for image blending.
bool Ditherer::atkinsonDither(std::vector<unsigned char>& data, int width, int height, int stride) {
    int threshold = 200;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int pixelVal = data[(y*stride) + x];

            int quantizedVal = (pixelVal < threshold) ? 0 : 255;
            data[(y*stride) + x] = static_cast<unsigned char>(quantizedVal);

            int error = pixelVal - quantizedVal;
            int diffusion = error/8;

            auto applySafeDiffusion = [&](int newX, int newY) {
                if (newX >= 0 && newX < width && newY >= 0 && newY < height) {
                    int diffusedVal = data[(newY*stride) + newX] + diffusion;
                    data[(newY*stride) + newX] = static_cast<unsigned char>(
                        std::clamp(diffusedVal, 0, 255)
                    );
                }
            };

            /* [...    ...  *  1/8 1/8]
            [...    1/8 1/8 1/8 ...]
            [...    ... 1/8 ... ...] */

            applySafeDiffusion(x+1, y);
            applySafeDiffusion(x+2, y);
            applySafeDiffusion(x-1, y+1);
            applySafeDiffusion(x, y+1);
            applySafeDiffusion(x+1, y+1);
            applySafeDiffusion(x, y+2);
        }
    }

    return true;
}