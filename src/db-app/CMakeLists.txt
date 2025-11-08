cmake_minimum_required(VERSION 3.12)

# set project name, version and language
project(db_app VERSION 1.0 LANGUAGES CXX)
# find_package(CURL REQUIRED)
# find_package(SQLite3 REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add cpp project files
add_executable(${PROJECT_NAME} db_app.cpp)

# set list of user static libs
set(STATIC_LIBS WMMLib SPALib solar_dbmanager APIManager)

# add header libs
target_include_directories(${PROJECT_NAME} PUBLIC 
                                ${CMAKE_CURRENT_SOURCE_DIR}/../../include 
                                ${CMAKE_CURRENT_SOURCE_DIR}/../../Data-Service/Data-API
                                ${CMAKE_CURRENT_SOURCE_DIR}/../../Database-Service
                                )

# link libs
target_link_libraries(${PROJECT_NAME} PRIVATE ${STATIC_LIBS} m)

if(EXISTS "WMM.COF")
    message(STATUS "WMM.COF File exists")
else()
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../../Tracking-Algorithm/wmm2025_Linux/data/WMM.COF DESTINATION .)
endif()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/../../Configs/sim.conf DESTINATION .)