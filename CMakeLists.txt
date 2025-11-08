cmake_minimum_required(VERSION 3.12)

# name of project
project(target VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# User-defined flags with defaults
option(BUILD_SENSOR_APP "Build sensor app" OFF) 
option(BUILD_API_APP "Build api app" OFF) 
option(BUILD_DB_APP "Build db app" OFF) 


# Configuration-specific flags
set(BUILD_TYPE "Release" CACHE STRING "Build type (Debug, Release)")
set(ENABLE_OPTIMIZATIONS OFF CACHE BOOL "Enable aggressive optimizations")

# add compilation folders
add_subdirectory(Tracking-Algorithm)
add_subdirectory(src)

if(BUILD_DB_APP)
  add_subdirectory(Data-Service)
  add_subdirectory(Database-Service)
endif()

# Print configuration summary
message(STATUS "Build configuration:")
message(STATUS "  Build Project SENSOR_APP: ${BUILD_SENSOR_APP}")
message(STATUS "  Build Project API_APP: ${BUILD_API_APP}")
message(STATUS "  Build Project DB_APP: ${BUILD_DB_APP}")
