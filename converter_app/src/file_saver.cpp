#include "file_saver.hpp"
#include <filesystem>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

bool FileSaver::savePreviewPNG(const std::vector<unsigned char>& ditheredData, int w, int h, int stride, std::string filename) {
    // stbi_write_png(path, width, height, channels, data, stride_in_bytes)
    bool success = stbi_write_png(filename.c_str(), w, h, 1, ditheredData.data(), stride);   
    return success;
}

bool FileSaver::clearFolder (const std::string& path) {
    if (path.empty()) {
        return false;
    }

    // Delete all files in path
    for (const auto& file : std::filesystem::directory_iterator(path)) {
        std::filesystem::remove_all(file.path());
    }
    return true;
}

bool FileSaver::checkFolder(const std::string& path) {
    if (std::filesystem::exists(path)) {
        return false;
    }
    std::filesystem::create_directories(path);
    return true;
}
