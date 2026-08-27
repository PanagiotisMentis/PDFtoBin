cmake_minimum_required(VERSION 3.10)

project(PDFtoBin)

find_package(PkgConfig)
pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)

add_executable(PDFtoBin main.cpp)
target_link_libraries(PDFtoBin PRIVATE PkgConfig::POPPLER_CPP)