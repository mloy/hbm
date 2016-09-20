project("hbm_communication_tool")
cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../../cmake)
find_package(JsonCpp)
if(JSONCPP_FOUND)
  include_directories( ${JSONCPP_INCLUDE_DIR} )
endif()

include_directories(../../include)

SET(CMAKE_CXX_FLAGS "-std=c++11 -fno-common -Wall -Werror -Wshadow -Wextra -Winit-self -Wstrict-overflow=5 -Wunused-result -Wcast-qual -Wcast-align -Wpointer-arith -Wformat=2 -Wwrite-strings -Wmissing-declarations -pedantic")


SET(NETADAPTERVIEW_SOURCE
    netadapterview.cpp
)

add_executable(
    netadapterview.bin
    ${NETADAPTERVIEW_SOURCE}
)

target_link_libraries( netadapterview.bin
    hbm
    ${JSONCPP_LIBRARY}
)

SET(NETLINKVIEW_SOURCE
    netlinkview.cpp
)

add_executable(
    netlinkview.bin
    ${NETLINKVIEW_SOURCE}
)

target_link_libraries( netlinkview.bin
    hbm
    ${JSONCPP_LIBRARY}
)

SET(TCPSERVER_SOURCE
    Server.cpp
)

add_executable(
    tcpserver.bin
    ${TCPSERVER_SOURCE}
)

target_link_libraries( tcpserver.bin
    hbm
    ${JSONCPP_LIBRARY}
)

SET(TCPCLIENT_SOURCE
    Client.cpp
)

add_executable(
    tcpclient.bin
    ${TCPCLIENT_SOURCE}
)

target_link_libraries( tcpclient.bin
    hbm
    ${JSONCPP_LIBRARY}
)
