cmake_minimum_required(VERSION 3.12)

# set project name, version and language
project(app VERSION 1.0 LANGUAGES CXX)

# set project version to C++ 14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add cpp project files
add_executable(${PROJECT_NAME} main.cpp)

# set list of user static libs
set(STATIC_LIBS WMMLib)

# add header libs
target_include_directories(${PROJECT_NAME} PUBLIC "../include")

# link user libs
target_link_libraries(${PROJECT_NAME} PRIVATE ${STATIC_LIBS})
target_link_libraries(${PROJECT_NAME} PRIVATE m)

if(EXISTS "WMM.COF")
    message(STATUS "WMM.COF File exists")
else()
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../../wmm2025_Linux/data/WMM.COF DESTINATION .)
endif()
