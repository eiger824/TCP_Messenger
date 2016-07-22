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
  ${PROJECT_SOURCE_DIR}/utils/src
  ${PROJECT_SOURCE_DIR}/utils/include
  )

set(HEADERS_MOC
  include/client.hpp
  include/chatwrapper.hpp
  include/chat.hpp
  include/message.hpp
  )
qt4_wrap_cpp(MOC_HEADERS ${HEADERS_MOC})

set(SOURCES
  src/client.cpp
  src/chatwrapper.cpp
  src/chat.cpp
  src/message.cpp
  )

add_executable(client
  src/main.cpp
  ${MOC_HEADERS}
  ${SOURCES}
  ${UTIL_HEADERS_MOC}
  ${MOC_UTIL_HEADERS}
  )

target_link_libraries(client
  Qt4::QtGui
  Qt4::QtXml
  Qt4::QtNetwork
  ${GLOG_LIBRARIES}
  Utils
  )

install(TARGETS client DESTINATION ${PROJECT_BINARY_DIR}/cmake_targets)
install(FILES
  ${PROJECT_SOURCE_DIR}/utils/images/online.png
  ${PROJECT_SOURCE_DIR}/utils/images/offline.png
  ${PROJECT_SOURCE_DIR}/utils/images/clock.png
  ${PROJECT_SOURCE_DIR}/utils/images/warning.png
  ${PROJECT_SOURCE_DIR}/utils/images/check.png
  ${PROJECT_SOURCE_DIR}/utils/images/seen.png
  DESTINATION ${CMAKE_INSTALL_PREFIX}/../images)

