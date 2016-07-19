cmake_minimum_required(VERSION 3.2)
find_package(Qt4 REQUIRED)
find_package(Glog REQUIRED)

include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

include_directories(system
  ${GLOG_INCLUDE_DIRS}
  )

set(HEADERS_MOC
  client.h
  )
qt4_wrap_cpp(MOC_HEADERS ${HEADERS_MOC})

set(SOURCES
  client.cpp
  )

add_executable(client
  main.cpp
  ${MOC_HEADERS}
  ${SOURCES}
  )

target_link_libraries(client
  Qt4::QtGui
  Qt4::QtXml
  Qt4::QtNetwork
  ${GLOG_LIBRARIES}
  )

install(TARGETS client DESTINATION ${PROJECT_BINARY_DIR}/cmake_targets)
