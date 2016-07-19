cmake_minimum_required(VERSION 2.6)
project(TCP_ClientServer)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ${TMP}")
add_subdirectory(server)
add_subdirectory(client)
