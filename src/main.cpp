#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <poppler/cpp/poppler-image.h>

#include "constants.hpp"
#include "bitpacking.hpp"
#include "dithering.hpp"
#include "fileutils.hpp"

using namespace bitpacking;
using namespace constants;
using namespace dithering;
using namespace fileutils;

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
    if (!fileutils::clearFolder(previewPath) || !fileutils::clearFolder(binPath)) {
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
        dithering::atkinsonDither(ditherData, w, h, stride);

        // Save Binary
        std::string binFilename = binPath + "music" + std::to_string(currentPage) + "_gxepd2.bin";
        bitpacking::packBinaryFile(w, h, stride, ditherData.data(), binFilename);

        // Save Preview
        std::string previewFilePath = previewPath + std::to_string(currentPage) + "preview.png";
        fileutils::savePreviewPNG(ditherData, w, h, stride, previewFilePath);

        // Inform Python GUI of progress
        std::cout << "PROGRESS_PAGE:" << currentPage + 1 << std::endl;
        
        delete pg;
    }

    delete doc;
    return 0;
}
