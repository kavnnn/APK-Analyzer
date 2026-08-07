cmake_minimum_required(VERSION 3.15)
project(apk-analyzer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

find_package(ZLIB REQUIRED)

add_executable(apk-analyzer
    src/main.cpp
    src/zip_reader.cpp
    src/axml_parser.cpp
)

target_include_directories(apk-analyzer PRIVATE include)
target_link_libraries(apk-analyzer PRIVATE ZLIB::ZLIB)
