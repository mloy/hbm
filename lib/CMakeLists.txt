project("hbm_lib" CXX)

cmake_minimum_required(VERSION 2.8)

set( INTERFACE_INCLUDE_DIR ../include/hbm)
include_directories(../include)

SET(EXTRA_FLAGS "-std=c++11 -fno-common -Wall -Werror -Wshadow -Wextra -Winit-self -Wstrict-overflow=5 -Wunused-result -Wcast-qual -Wcast-align -Wpointer-arith -Wformat=2 -Wwrite-strings -Wmissing-declarations -pedantic")

SET( HBMLIB_INTERFACE_HEADERS
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
)

set_source_files_properties(${HBMLIB_SOURCES} PROPERTIES COMPILE_FLAGS "${EXTRA_FLAGS}")

add_library(hbm SHARED ${HBMLIB_SOURCES})

set_target_properties(
  hbm
  PROPERTIES
  SOVERSION 0
  VERSION 0.0.5
  )

install(TARGETS hbm LIBRARY DESTINATION lib)
install (DIRECTORY ${INTERFACE_INCLUDE_DIR} DESTINATION include)
