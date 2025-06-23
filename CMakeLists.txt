cmake_minimum_required(VERSION 3.20)

project(git-starter-cpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
#set(CMAKE_TOOLCHAIN_FILE "./vcpkg.cmake" CACHE STRING "")
set(CPR_DIR "/app/build/vcpkg/installed/x64-linux/share/cpr")

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

set ( CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} /usr/local/lib/cmake)
find_package ( CPR REQUIRED )
include_directories ( ${CPR_INCLUDE_DIRS} )

add_executable(git ${SOURCE_FILES})

#target_link_libraries(git -lz)
#
target_link_libraries(git PRIVATE OpenSSL::Crypto)
target_link_libraries(git PRIVATE ZLIB::ZLIB)
target_link_libraries(git PRIVATE CPR::CPR)