cmake_minimum_required(VERSION 3.10)
project(PDFtoBin)

find_package(PkgConfig)
pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)

include_directories(include)

add_executable(PDFtoBin src/main.cpp src/bitpacking.cpp src/dithering.cpp src/fileutils.cpp)
target_link_libraries(PDFtoBin PRIVATE PkgConfig::POPPLER_CPP)