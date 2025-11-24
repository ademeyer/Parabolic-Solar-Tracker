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
option(BUILD_PLATE_TEST "Build collector test" OFF) 
option(BUILD_RUNNER_APP "Build Analysis Runner Tool" OFF) 


# Configuration-specific flags
set(BUILD_TYPE "Release" CACHE STRING "Build type (Debug, Release)")
set(ENABLE_OPTIMIZATIONS OFF CACHE BOOL "Enable aggressive optimizations")

# add compilation folders
add_subdirectory(Tracking-Algorithm)


if(BUILD_DB_APP)
  add_subdirectory(Data-Service)
endif()

if(BUILD_DB_APP OR BUILD_RUNNER_APP)
  add_subdirectory(Database-Service)
endif()

if(BUILD_PLATE_TEST OR BUILD_RUNNER_APP)
  add_subdirectory(Modelling)
  add_subdirectory(Collector)
endif()

add_subdirectory(src)

# Print configuration summary
message(STATUS "Build configuration:")
message(STATUS "  Build Project SENSOR_APP: ${BUILD_SENSOR_APP}")
message(STATUS "  Build Project API_APP: ${BUILD_API_APP}")
message(STATUS "  Build Project DB_APP: ${BUILD_DB_APP}")
message(STATUS "  Build Project PLATE_TEST: ${BUILD_PLATE_TEST}")
message(STATUS "  Build Project Analysis Runner Tool: ${BUILD_RUNNER_APP}")
