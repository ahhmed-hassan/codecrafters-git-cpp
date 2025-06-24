# Minimum required version of CMake
cmake_minimum_required(VERSION 3.20)

# Project name and settings
project(git-starter-cpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard

# Add path for custom Find modules
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Set options if needed (e.g., USE_SYSTEM_GTEST)
set(USE_SYSTEM_GTEST ON CACHE BOOL "Use system-installed GTest")

# Locate dependencies
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(cpr CONFIG REQUIRED)

# Include directories provided by dependencies
include_directories(${CPR_INCLUDE_DIRS})

# Source files globbing (adjust paths as needed)
file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

# Define executable target and link libraries
add_executable(git ${SOURCE_FILES})
target_link_libraries(git PRIVATE OpenSSL::Crypto) 
target_link_libraries(git PRIVATE ZLIB::ZLIB )
target_link_libraries(git PRIVATE cpr::cpr )