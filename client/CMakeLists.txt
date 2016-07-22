cmake_minimum_required(VERSION 2.6)
find_package(Qt4 REQUIRED)
find_package(Glog REQUIRED)

include(${QT_USE_FILE})
add_definitions(${QT_DEFINITIONS})

include_directories(system
  ${PROJ_INCLUDES}
  ${GLOG_INCLUDE_DIRS}
  )

set(HEADERS_MOC
  client.hpp
  chatwrapper.hpp
  chat.hpp
  message.hpp
  )
qt4_wrap_cpp(MOC_HEADERS ${HEADERS_MOC})

set(SOURCES
  client.cpp
  chatwrapper.cpp
  chat.cpp
  message.cpp
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
install(FILES
  ${PROJECT_SOURCE_DIR}/images/online.png
  ${PROJECT_SOURCE_DIR}/images/offline.png
  ${PROJECT_SOURCE_DIR}/images/clock.png
  ${PROJECT_SOURCE_DIR}/images/warning.png
  ${PROJECT_SOURCE_DIR}/images/check.png
  ${PROJECT_SOURCE_DIR}/images/seen.png
  DESTINATION ${CMAKE_INSTALL_PREFIX}/../images)

