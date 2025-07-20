cmake_minimum_required(VERSION 3.12)

# name of project
project(SPALib VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} STATIC 
                          SPALib.cpp 
                          ${CMAKE_CURRENT_SOURCE_DIR}/../../../solar-tracker/app/spa/spa.c
                          )

# add include file
target_include_directories(${PROJECT_NAME} PUBLIC
                                            ${CMAKE_CURRENT_SOURCE_DIR}               # Current directory (.)
                                            ${CMAKE_CURRENT_SOURCE_DIR}/../
                                            ${CMAKE_CURRENT_SOURCE_DIR}/../../../solar-tracker/app/spa/ # Parent directory (../)
                            )

target_link_libraries(${PROJECT_NAME} PRIVATE m) # Math lib

# Compiler options
target_compile_options(${PROJECT_NAME}  PRIVATE
    -Wall # show all warnings
    -Wextra
    # -Werror
)