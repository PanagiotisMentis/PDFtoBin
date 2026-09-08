#pragma once

#include <vector>
#include <string>

class FileSaver {
    public:
        bool savePreviewPNG(
            const std::vector<unsigned char>& ditheredData, 
            int w, 
            int h, 
            int stride, 
            std::string filename
        );
        bool clearFolder (const std::string& path);
        bool checkFolder(const std::string& path);
};