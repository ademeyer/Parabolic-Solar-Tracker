cmake_minimum_required(VERSION 3.12)
project(APIManager VERSION 1.0 LANGUAGES CXX)

find_package(CURL REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} STATIC APIManager.cpp)
# add header libs
target_include_directories(${PROJECT_NAME} PUBLIC 
                                  ${CMAKE_CURRENT_SOURCE_DIR}
                                ${CMAKE_CURRENT_SOURCE_DIR}/../../include
                                )

target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)


# Compiler options
target_compile_options(${PROJECT_NAME}  PRIVATE
    -Wall # show all warnings
    -Wextra
)

