cmake_minimum_required(VERSION 3.12)
project(src VERSION 1.0 LANGUAGES CXX)

if(BUILD_DB_APP)
  add_subdirectory(db-app)
endif()

if(BUILD_API_APP)
  add_subdirectory(api-test)
endif()

if(BUILD_SENSOR_APP)
  add_subdirectory(mock-sensor)
endif()