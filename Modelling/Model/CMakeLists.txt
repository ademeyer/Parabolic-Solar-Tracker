cmake_minimum_required(VERSION 3.12)

project(soltrace_model VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(STATIC_LIBS coretrace)

add_library(${PROJECT_NAME} STATIC SolTraceModel.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC 
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CMAKE_CURRENT_SOURCE_DIR}/../../Include
                            ${CMAKE_CURRENT_SOURCE_DIR}/../../Include/External/SolTrace/coretrace
                            )

target_link_libraries(${PROJECT_NAME} PRIVATE ${STATIC_LIBS} m)

# Compiler options
target_compile_options(${PROJECT_NAME}  PRIVATE
    -Wall # show all warnings
    -Wextra
)