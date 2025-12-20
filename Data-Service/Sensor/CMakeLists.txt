cmake_minimum_required(VERSION 3.12)

project(sensor VERSION 1.0 LANGUAGES CXX)

# add header libs
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../Include)
