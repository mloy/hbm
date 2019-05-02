project("hbm_lib" CXX)

cmake_minimum_required(VERSION 2.8)

set( INTERFACE_INCLUDE_DIR ../include/hbm)
include_directories(../include)


if(ROOT_SEARCH_PATH)
  list(APPEND CMAKE_PREFIX_PATH ${ROOT_SEARCH_PATH}/usr)
  set( PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON)
endif()


SET(EXTRA_FLAGS "-std=c++11 -fno-common -Wall -Werror -Wshadow -Wextra -Winit-self -Wstrict-overflow=5 -Wunused-result -Wcast-qual -Wcast-align -Wpointer-arith -Wformat=2 -Wwrite-strings -Wmissing-declarations -pedantic")

SET( HBMLIB_INTERFACE_HEADERS
    ${INTERFACE_INCLUDE_DIR}/communication/ipv4address.h
    ${INTERFACE_INCLUDE_DIR}/communication/ipv6address.h
    ${INTERFACE_INCLUDE_DIR}/communication/bufferedreader.h
    ${INTERFACE_INCLUDE_DIR}/communication/multicastserver.h
    ${INTERFACE_INCLUDE_DIR}/communication/netadapter.h
    ${INTERFACE_INCLUDE_DIR}/communication/netadapterlist.h
    ${INTERFACE_INCLUDE_DIR}/communication/netlink.h
    ${INTERFACE_INCLUDE_DIR}/communication/socketnonblocking.h
    ${INTERFACE_INCLUDE_DIR}/communication/tcpserver.h
    ${INTERFACE_INCLUDE_DIR}/debug/stack_trace.hpp
    ${INTERFACE_INCLUDE_DIR}/exception/errno_exception.hpp
    ${INTERFACE_INCLUDE_DIR}/exception/exception.hpp
    ${INTERFACE_INCLUDE_DIR}/exception/jsonrpc_exception.h
    ${INTERFACE_INCLUDE_DIR}/jsonrpc/jsonrpc_defines.h
    ${INTERFACE_INCLUDE_DIR}/string/replace.h
    ${INTERFACE_INCLUDE_DIR}/string/trim.h
    ${INTERFACE_INCLUDE_DIR}/string/split.h
    ${INTERFACE_INCLUDE_DIR}/sys/defines.h
    ${INTERFACE_INCLUDE_DIR}/sys/eventloop.h
    ${INTERFACE_INCLUDE_DIR}/sys/executecommand.h
    ${INTERFACE_INCLUDE_DIR}/sys/notifier.h
    ${INTERFACE_INCLUDE_DIR}/sys/pidfile.h
    ${INTERFACE_INCLUDE_DIR}/sys/timer.h
)

set(HBMLIB_SOURCES
  ${HBMLIB_INTERFACE_HEADERS}
  communication/ipv4address.cpp
  communication/ipv6address.cpp
  communication/linux/bufferedreader.cpp
  communication/linux/multicastserver.cpp
  communication/linux/netadapter.cpp
  communication/linux/netadapterlist.cpp
  communication/linux/netlink.cpp
  communication/linux/socketnonblocking.cpp
  communication/linux/tcpserver.cpp
  exception/jsonrpc_exception.cpp
  string/replace.cpp
  string/split.cpp
  string/trim.cpp
  sys/linux/eventloop.cpp
  sys/linux/executecommand.cpp
  sys/linux/notifier.cpp
  sys/linux/timer.cpp
  sys/pidfile.cpp
)

set_source_files_properties(${HBMLIB_SOURCES} PROPERTIES COMPILE_FLAGS "${EXTRA_FLAGS}")

add_library(hbm SHARED ${HBMLIB_SOURCES})
add_library(hbm-static STATIC ${HBMLIB_SOURCES})


SET( HBM_MAJOR_VERSION 1)
SET( HBM_MINOR_VERSION 0)
SET( HBM_TAG_NUMBER 33)

SET( HBM_VERSION ${HBM_MAJOR_VERSION}.${HBM_MINOR_VERSION}.${HBM_TAG_NUMBER} )

set_target_properties(
  hbm
  PROPERTIES
  SOVERSION ${HBM_MAJOR_VERSION}
  VERSION ${HBM_VERSION}
  )

set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix "\${prefix}")
set(libdir "\${exec_prefix}/lib")
set(includedir "\${prefix}/include")
configure_file("${PROJECT_SOURCE_DIR}/pkg-config/hbm.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/pkg-config/hbm.pc" @ONLY)
configure_file("${PROJECT_SOURCE_DIR}/version.h.in" "${PROJECT_BINARY_DIR}/version.h")


INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/pkg-config/hbm.pc" DESTINATION lib/pkgconfig)

install(TARGETS hbm LIBRARY DESTINATION lib)
install(TARGETS hbm-static ARCHIVE DESTINATION lib)
install (DIRECTORY ${INTERFACE_INCLUDE_DIR} DESTINATION include)
install (FILES "${PROJECT_BINARY_DIR}/version.h" DESTINATION include/hbm/)
