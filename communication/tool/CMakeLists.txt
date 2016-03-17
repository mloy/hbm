project("hbm_communication_tool")
cmake_minimum_required(VERSION 2.8)

include_directories(../../..)

SET(CMAKE_CXX_FLAGS "-std=c++11 -fno-common -Wall -Werror -Wshadow -Wextra -Winit-self -Wstrict-overflow=5 -Wunused-result -Wcast-qual -Wcast-align -Wpointer-arith -Wformat=2 -Wwrite-strings -Wmissing-declarations -pedantic")

SET(NETLINKVIEW_SOURCE
    ../linux/netadapter.cpp
    ../linux/netadapterlist.cpp
    ../linux/netlink.cpp
    ../../sys/linux/eventloop.cpp
    netlinkview.cpp
)

add_executable(
    netlinkview.bin
    ${NETLINKVIEW_SOURCE}
)

SET(TCPSERVER_SOURCE
    ../linux/bufferedreader.cpp
    ../linux/tcpserver.cpp
    ../linux/socketnonblocking.cpp
    ../../sys/linux/eventloop.cpp
    Server.cpp
)

add_executable(
    tcpserver.bin
    ${TCPSERVER_SOURCE}
)

SET(TCPCLIENT_SOURCE
    ../linux/bufferedreader.cpp
    ../linux/socketnonblocking.cpp
    ../../sys/linux/eventloop.cpp
    Client.cpp
)

add_executable(
    tcpclient.bin
    ${TCPCLIENT_SOURCE}
)
