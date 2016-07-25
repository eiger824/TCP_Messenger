cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)
find_package(Glog REQUIRED)

include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

include_directories(system
  ${PROJ_INCLUDES}
  ${GLOG_INCLUDE_DIRS}
  )
include_directories(
  src
  include
  )

set(UTILS_HEADERS_MOC
  include/definitions.hpp
  include/protocol.hpp
  include/bits.hpp
  )
qt4_wrap_cpp(MOC_UTIL_HEADERS ${UTILS_HEADERS_MOC})

set(UTIL_SOURCES
  src/protocol.cpp
  src/bits.cpp
  )
add_library(Utils
  ${MOC_UTILS_HEADERS}
  ${UTIL_SOURCES}
  )

target_link_libraries(Utils
  Qt4::QtGui
  Qt4::QtXml
  ${GLOG_LIBRARIES}
  )
