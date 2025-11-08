cmake_minimum_required(VERSION 3.12)

# set project name, version and language
project(mock_sensor VERSION 1.0 LANGUAGES CXX)

# add cpp project files
add_executable(${PROJECT_NAME} mock_sensor.cpp)

# set list of user static libs
set(STATIC_LIBS WMMLib SPALib)

# add header libs
target_include_directories(${PROJECT_NAME} PUBLIC 
                                ${CMAKE_CURRENT_SOURCE_DIR}/../../include 
                                ${CMAKE_CURRENT_SOURCE_DIR}/../../Data-Service/Sensor
                                )

# link libs
target_link_libraries(${PROJECT_NAME} PRIVATE ${STATIC_LIBS} m)

if(EXISTS "WMM.COF")
    message(STATUS "WMM.COF File exists")
else()
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../../Tracking-Algorithm/wmm2025_Linux/data/WMM.COF DESTINATION .)
endif()
