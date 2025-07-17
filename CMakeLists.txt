cmake_minimum_required(VERSION 3.12)

# name of project
project(all VERSION 1.0 LANGUAGES CXX)

# set project version to C++ 14
set(CMAKE_CXX_STANDARD 14)

# set project to debug configuration
set(CMAKE_BUILD_TYPE Debug)

# add sub dirs
add_subdirectory(project/include)

# add main src
add_subdirectory(project/src)