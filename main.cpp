#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>

#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <poppler/cpp/poppler-image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

const int TARGET_WIDTH = 680;
const int TARGET_HEIGHT = 960;



// -----OBSOLETE--------------------------------
// Old function that takes a C-Style Array with width, height, stride, and an output array name and preview filename.
// Returns true upon success and outputs a C-Style array of image data to a new file. 
bool packCStyleArray(int width, int height, int stride, const unsigned char* data, std::string filename, std::string arrayname) {
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
//------------------------------------


// Takes a pointer to an image data char array with height, width, 
// and stride (bytes per row) and output filename.
// Returns true upon success and creates a .bin file of parsed image data.
bool packBinaryFile(int width, int height, int stride, const unsigned char* data, std::string filename) {
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


// Takes a vector (1d) of copied image data. Basically an array of bytes for each pixel. 
// height, width, and stride (bytes per row).
// Returns true upon success, modifies the image data in place to apply Atkinson Dithering for image blending.
bool atkinsonDither(std::vector<unsigned char>& data, int width, int height, int stride) {
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

// Takes 
bool savePreviewPNG(const std::vector<unsigned char>& ditheredData, int w, int h, int stride, std::string filename) {
    // stbi_write_png(path, width, height, channels, data, stride_in_bytes)
    bool success = stbi_write_png(filename.c_str(), w, h, 1, ditheredData.data(), stride);   
    return success;
}

bool clearFolder (const std::string& path) {
    if (path.empty()) {
        return false;
    }

    // Delete all files in path
    for (const auto& file : std::filesystem::directory_iterator(path)) {
        std::filesystem::remove_all(file.path());
    }
    return true;
}

bool checkFolder(const std::string& path) {
    if (std::filesystem::exists(path)) {
        return false;
    }
    std::filesystem::create_directories(path);
    return true;
}

int main(int argc, char* argv[]) {
    // Main is called from a Python GUI, so parameters are passed through argv[].

    // --- PATH LOGIC UPDATE ---
    std::string inputPdfPath = "../pdf.pdf"; // Default for dev
    std::string baseOutputPath = "../";      // Default for dev

    // If Python provides arguments: argv[1] = output_dir, argv[2] = input_pdf
    if (argc >= 3) {
        baseOutputPath = argv[1];
        inputPdfPath = argv[2];
        
        // Ensure output path ends with a slash
        if (baseOutputPath.back() != '/' && baseOutputPath.back() != '\\') {
            baseOutputPath += "/";
        }
    }

    // Load the document from the path provided
    auto* doc = poppler::document::load_from_file(inputPdfPath);
    if (!doc) {
        std::cerr << "Failed to load PDF at: " << inputPdfPath << std::endl;
        return 1;
    }

    auto pageCount = doc->pages();

    // Define subfolders based on the base output path
    std::string previewPath = baseOutputPath + "previews/";
    std::string binPath = baseOutputPath + "binfiles/";

    // Create folders if they don't exist
    std::filesystem::create_directories(previewPath);
    std::filesystem::create_directories(binPath);

    // Clear existing files
    if (!clearFolder(previewPath) || !clearFolder(binPath)) {
        std::cerr << "Error clearing output folders." << std::endl;
        return 1;
    }

    // Inform Python GUI of total pages
    std::cout << "TOTAL_PAGES:" << pageCount << std::endl;

    // Loop through each page
    for (int currentPage = 0; currentPage < pageCount; ++currentPage) {
        auto* pg = doc->create_page(currentPage);
        if (!pg) {
            return 1; // C++ error code for main
        }

        poppler::page_renderer renderer;
        renderer.set_image_format(poppler::image::format_gray8);

        auto dpiX = (static_cast<double>(TARGET_WIDTH) / pg->page_rect().width()) * 72.0;
        auto dpiY = (static_cast<double>(TARGET_HEIGHT) / pg->page_rect().height()) * 72.0;
        
        auto img = renderer.render_page(pg, dpiX, dpiY);

        int w = img.width();
        int h = img.height();

        // Memory width of 1 row
        int stride = img.bytes_per_row();

        // Make a COPY of the image data.
        // Notice how it's initializew with parenthesis: (first pointer, last pointer)
        // NOT with brackets: {first pointer, last pointer}.
        std::vector<unsigned char> ditherData(
            // Pointer to the first pixel char
            reinterpret_cast<const unsigned char*>(img.const_data()), 

            // Pointer to the last pixel char
            // (first pixel pointer) + (height * width of a row)
            reinterpret_cast<const unsigned char*>(img.const_data() + (h * stride))
        );

        // Modify ditherData in place and apply dithering
        atkinsonDither(ditherData, w, h, stride);

        // Save Binary
        std::string binFilename = binPath + "music" + std::to_string(currentPage) + "_gxepd2.bin";
        packBinaryFile(w, h, stride, ditherData.data(), binFilename);

        // Save Preview
        std::string previewFilePath = previewPath + std::to_string(currentPage) + "preview.png";
        savePreviewPNG(ditherData, w, h, stride, previewFilePath);

        // Inform Python GUI of progress
        std::cout << "PROGRESS_PAGE:" << currentPage + 1 << std::endl;
        
        delete pg;
    }

    delete doc;
    return 0;
}
