cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)
find_package(Glog REQUIRED)

include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

include_directories(system
  ${GLOG_INCLUDE_DIRS}
  )

set(HEADERS_MOC
  server.h
  )
qt4_wrap_cpp(MOC_HEADERS ${HEADERS_MOC})

set(SOURCES
  server.cpp
  )

add_executable(server
  main.cpp
  ${MOC_HEADERS}
  ${SOURCES}
  )

target_link_libraries(server
  Qt4::QtGui
  Qt4::QtXml
  Qt4::QtNetwork
  ${GLOG_LIBRARIES}
  )

install(TARGETS server DESTINATION ${PROJECT_BINARY_DIR}/cmake_targets)
