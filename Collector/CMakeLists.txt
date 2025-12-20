cmake_minimum_required(VERSION 3.12)

project(collector VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} STATIC Collector.cpp)

set(STATIC_LIBS soltrace_model SPALib)

target_include_directories(${PROJECT_NAME} PUBLIC 
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CMAKE_CURRENT_SOURCE_DIR}/../Include 
                          ${CMAKE_CURRENT_SOURCE_DIR}/../Modelling/Model
                          ${CMAKE_CURRENT_SOURCE_DIR}/../Modelling/SolTrace/coretrace
                          ${CMAKE_CURRENT_SOURCE_DIR}/../Tracking-Algorithm/SPALibs
                          ${CMAKE_CURRENT_SOURCE_DIR}/../Tracking-Algorithm/solar-tracker/app/spa
                            )

target_link_libraries(${PROJECT_NAME} PRIVATE ${STATIC_LIBS} m)

# Compiler options
target_compile_options(${PROJECT_NAME}  PRIVATE
    -Wall # show all warnings
    -Wextra
)