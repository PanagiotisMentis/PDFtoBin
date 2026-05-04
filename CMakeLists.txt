cmake_minimum_required(VERSION 3.10)

project(PDFtoCStyle)

find_package(PkgConfig)
pkg_check_modules(POPPLER_CPP REQUIRED IMPORTED_TARGET poppler-cpp)

add_executable(PDFtoCStyle main.cpp)
target_link_libraries(PDFtoCStyle PRIVATE PkgConfig::POPPLER_CPP)