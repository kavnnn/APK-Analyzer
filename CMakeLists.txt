cmake_minimum_required(VERSION 3.15)
project(apk-analyzer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Vendor zlib via FetchContent instead of relying on a system package.
# This keeps the build identical (and dependency-free) across Linux,
# macOS, and Windows CI runners - no apt/brew/vcpkg steps needed.
include(FetchContent)
set(SKIP_INSTALL_ALL ON CACHE BOOL "" FORCE)
set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    zlib
    GIT_REPOSITORY https://github.com/madler/zlib.git
    GIT_TAG        v1.3.1
)
FetchContent_MakeAvailable(zlib)

add_executable(apk-analyzer
    src/main.cpp
    src/zip_reader.cpp
    src/axml_parser.cpp
)

target_include_directories(apk-analyzer PRIVATE
    include
    ${zlib_SOURCE_DIR}
    ${zlib_BINARY_DIR}
)

target_link_libraries(apk-analyzer PRIVATE zlibstatic)
