cmake_minimum_required(VERSION 3.12)

# set project name, version and language
project(api_test VERSION 1.0 LANGUAGES CXX)

find_package(CURL REQUIRED)

# add cpp project files
add_executable(${PROJECT_NAME} api_test.cpp)

# set list of user static libs
set(STATIC_LIBS WMMLib SPALib)

# add header libs
target_include_directories(${PROJECT_NAME} PUBLIC 
                                ${CMAKE_CURRENT_SOURCE_DIR}/../../include 
                                ${CMAKE_CURRENT_SOURCE_DIR}/../../Data-Service/Data-API
                                )

# link libs
target_link_libraries(${PROJECT_NAME} PRIVATE ${STATIC_LIBS} m CURL::libcurl)

if(EXISTS "WMM.COF")
    message(STATUS "WMM.COF File exists")
else()
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../../Tracking-Algorithm/wmm2025_Linux/data/WMM.COF DESTINATION .)
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../../Configs/sim.conf DESTINATION .)