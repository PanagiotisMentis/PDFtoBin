#include <iostream>
#include <fstream>
#include <iomanip>

#include "bit_packer.hpp"
#include "constants.hpp"
using namespace constants;

// Takes a pointer to an image data char array with height, width, 
// and stride (bytes per row) and output filename.
// Returns true upon success and creates a .bin file of parsed image data.
bool BitPacker::packBinaryFile(int width, int height, int stride, const unsigned char* data, std::string filename) {
    std::ofstream outFile(filename, std::ios::binary);
    
    if (!outFile) {
        std::cerr << "Error: Could not open file: " << filename << std::endl;
        return false;
    }

    int bytesPerPixel = stride / width;
    int byteCount = 0;

    // Vertically pack bits into a .bin file for GXEPD2 Rendering.
    for (int x = width - 1; x >= 0; --x) {
        
        // Squash a vertical strip of 8 bits into currentByte
        for (int y = 0; y < height; y += 8) {
            unsigned char currentByte = 0;

            // Get exact vertical bit within the current pixel.
            for (int bit = 0; bit < 8; ++bit) {
                int targetY = y + bit;
                
                if (targetY < height) {
                    // Pointer to the actual pixel at this vertical bit.
                    const unsigned char* pixelPtr = data + (targetY * stride) + (x * bytesPerPixel);
                    
                    // If pixelval is dark enough
                    if (*pixelPtr < 128) { 
                        // Bitwise OR [currentByte] with [00000001 shifted left by (bit) bits]
                        // => store result in currentByte.
                        currentByte |= (1 << bit); 
                    }
                }
            }
            outFile.put(static_cast<char>(currentByte));
            byteCount++;
        }
    }

    outFile.close();
    // std::cout << "Generated " << filename << " (" << byteCount << " bytes)." << std::endl;
    return true;
}

// -----OBSOLETE--------------------------------
// Old function that takes a C-Style Array with width, height, stride, and an output array name and preview filename.
// Returns true upon success and outputs a C-Style array of image data to a new file. 
bool BitPacker::packCStyleArray(int width, int height, int stride, const unsigned char* data, std::string filename, std::string arrayname) {
    int bytesPerPixel = stride / width;

    std::ofstream outFile(filename);
    outFile << "const unsigned char " << arrayname << "[] PROGMEM = {\n";

    // Vertically pack bits into C-style array for GXEPD2 Rendering.
    int byteCount = 0;
    for (int pixelX = TARGET_WIDTH; pixelX >= 0; --pixelX) {
        for (int pixelY = 0; pixelY < TARGET_HEIGHT; pixelY += 8) {
            unsigned char currentByte = 0;

            // Squash a vertical strip of 8 bits into currentByte
            for (int bit = 0; bit < 8; ++bit) {
                // Get exact vertical bit within the current pixel.
                int bitY = pixelY + bit;
                // Pointer to the actual pixel at this vertical bit.
                auto* verticalBitsPixel = data + (bitY * stride) + (pixelX * bytesPerPixel);
                
                if (*verticalBitsPixel < 128) {
                    // Bitwise OR [currentByte] with [00000001 shifted left by (7 - bit) bits]
                    // => store result in currentByte.
                    currentByte |= (1 << (7-bit)); // Pack the MSB
                }
            }

            outFile << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)currentByte << ", ";
            if (++byteCount % 12 == 0) {
                outFile << "\n  ";
            }
        }
    }

    outFile << "\n};";
    std::cout << "Generated header with " << byteCount << " bytes." << std::endl;
    return true;
}
