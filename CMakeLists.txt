cmake_minimum_required(VERSION 2.6)
project(messenger)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ${TMP}")

add_subdirectory(utils)
add_subdirectory(server)
add_subdirectory(client)
