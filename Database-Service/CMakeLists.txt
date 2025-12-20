cmake_minimum_required(VERSION 3.12)

find_package(SQLite3 REQUIRED)

# set project name, version and language
project(solar_dbmanager VERSION 1.0 LANGUAGES CXX)

# set project version to C++ 20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} STATIC SolarDatabaseManager.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC 
                                                ${CMAKE_CURRENT_SOURCE_DIR}
                                                ${CMAKE_CURRENT_SOURCE_DIR}/../Include
                                                )

target_link_libraries(${PROJECT_NAME} PRIVATE SQLite::SQLite3)

# Compiler options
target_compile_options(${PROJECT_NAME}  PRIVATE
    -Wall # show all warnings
    -Wextra
)