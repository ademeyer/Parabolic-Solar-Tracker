cmake_minimum_required(VERSION 3.12)

# name of project
project(WMMLib VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# add lib file
file(GLOB WMM_C_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/../../../wmm2025_Linux/src/*.c)

add_library(${PROJECT_NAME} STATIC 
                          WMMLib.cpp 
                          ${WMM_C_SOURCES}
                          )

# add include file
target_include_directories(${PROJECT_NAME} PUBLIC
                                            ${CMAKE_CURRENT_SOURCE_DIR}               # Current directory (.)
                                            ${CMAKE_CURRENT_SOURCE_DIR}/..            # Parent directory (../)
                                            ${CMAKE_CURRENT_SOURCE_DIR}/../../../wmm2025_Linux/src  # Additional 
                            )

target_link_libraries(${PROJECT_NAME} PRIVATE m) # Math lib

# Compiler options
target_compile_options(${PROJECT_NAME}  PRIVATE
    -Wall # show all warnings
    -Wextra
    # -Werror
)