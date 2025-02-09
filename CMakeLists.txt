cmake_minimum_required(VERSION 3.13)

project(git-starter-cpp)

add_subdirectory(zstr)
target_link_libraries(git-starter-cpp PRIVATE zstr::zstr)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

add_executable(git ${SOURCE_FILES})

target_link_libraries(git -lz)
