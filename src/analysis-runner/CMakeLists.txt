cmake_minimum_required(VERSION 3.21)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--rpath=/usr/lib/x86_64-linux-gnu")
set(CMAKE_SKIP_RPATH TRUE)

# set project name, version and language
project(analysis_runner VERSION 1.0 LANGUAGES CXX)

find_package(Python3 3.10 EXACT COMPONENTS Development Interpreter REQUIRED)

# add cpp project files
add_executable(${PROJECT_NAME} analysis_runner.cpp)

# set list of user static libs
set(STATIC_LIBS collector solar_dbmanager)

# add header libs
target_include_directories(${PROJECT_NAME} PUBLIC 
                                ${CMAKE_CURRENT_SOURCE_DIR}/../../include 
                                ${CMAKE_CURRENT_SOURCE_DIR}/../../Database-Service
                                ${CMAKE_CURRENT_SOURCE_DIR}/../../Modelling/Model
                                ${CMAKE_CURRENT_SOURCE_DIR}/../../Modelling/Renderer
                                ${CMAKE_CURRENT_SOURCE_DIR}/../../Modelling/Analyzer
                                ${CMAKE_CURRENT_SOURCE_DIR}/../../Modelling/Renderer/matplotlib-cpp
                                ${CMAKE_CURRENT_SOURCE_DIR}/../../Collector
                                #"~/anaconda3/lib/python3.13/site-packages/numpy/_core/include"
                                )

target_link_libraries(${PROJECT_NAME} PRIVATE ${STATIC_LIBS} m Python3::Python)


target_compile_options(${PROJECT_NAME}  PRIVATE
    -Wall # show all warnings
    -Wextra
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../../Tracking-Algorithm/wmm2025_Linux/data/WMM.COF DESTINATION .)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../../Configs/material.conf DESTINATION .)
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../../Configs/run.conf DESTINATION .)

