cmake_minimum_required(VERSION 3.2)
project(TCP_ClientServer)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/CMakeModules;${CMAKE_MODULE_PATH}")

add_subdirectory(server)
add_subdirectory(client)
