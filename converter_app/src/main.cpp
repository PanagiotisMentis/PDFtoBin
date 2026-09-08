#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-page-renderer.h>
#include <poppler/cpp/poppler-image.h>

#include "bit_packer.hpp"
#include "ditherer.hpp"
#include "file_saver.hpp"

#include "constants.hpp"
using namespace constants;

// TODO
// -----------------
// * Wrap in HTTP Python server, so ESP32 can pull .bin files
// * Clean up logic here for more readability

int main(int argc, char* argv[]) {
    // Main is called from a Python GUI, so parameters are passed through argv[].

    Ditherer ditherer;
    FileSaver fileSaver;
    BitPacker bitPacker;

    std::string inputPdfPath = "../pdf.pdf"; // Default for dev
    std::string baseOutputPath = "../";      // Default for dev

    // If Python (or command line) provides arguments: argv[1] = output_dir, argv[2] = input_pdf
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
    if (!fileSaver.clearFolder(previewPath) || !fileSaver.clearFolder(binPath)) {
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

        // Use poppler to make renderer and pages
        poppler::page_renderer renderer;
        renderer.set_image_format(poppler::image::format_gray8);

        // Horizontal and Vertical DPI needed to scale a page to TARGET_WIDTH and TARGET_HEIGHT (with respect to 72 DPI).
        // 72.0 DPI is digital standard (PDF).
        //
        // If dpiX and Y are above 72.0, the image is too big and needs to be scaled down (more dots per inch).
        // If dpiX and Y are below 72.0, the image is too small and needs to be scaled up (fewer dots per inch).
        // If dpiX and Y are exactly 72.0, the image is the right size for TARGET_WIDTH and TARGET_HEIGHT.
        auto dpiX = (static_cast<double>(TARGET_WIDTH) / pg->page_rect().width()) * 72.0;
        auto dpiY = (static_cast<double>(TARGET_HEIGHT) / pg->page_rect().height()) * 72.0;
        
        // Render a page with the x resolution and y resolution as dpiX and dpiYs
        auto img = renderer.render_page(pg, dpiX, dpiY);

        int w = img.width();
        int h = img.height();

        // Stride is the memory width (bytes) of 1 row
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
        ditherer.atkinsonDither(ditherData, w, h, stride);

        // Save Binary
        std::string binFilename = binPath + "music" + std::to_string(currentPage) + "_gxepd2.bin";
        bitPacker.packBinaryFile(w, h, stride, ditherData.data(), binFilename);

        // Save Preview
        std::string previewFilePath = previewPath + std::to_string(currentPage) + "preview.png";
        fileSaver.savePreviewPNG(ditherData, w, h, stride, previewFilePath);

        // Inform Python GUI of progress
        std::cout << "PROGRESS_PAGE:" << currentPage + 1 << std::endl;
        
        delete pg;
    }

    delete doc;
    return 0;
}
