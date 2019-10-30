project("hbm_communication_tool")
cmake_minimum_required(VERSION 2.8)


include_directories(../../include)

SET(CMAKE_CXX_FLAGS "-std=c++11 -fno-common -Wall -Werror -Wshadow -Wextra -Winit-self -Wstrict-overflow=5 -Wunused-result -Wcast-qual -Wcast-align -Wpointer-arith -Wformat=2 -Wwrite-strings -Wmissing-declarations -pedantic")


SET(NETADAPTERVIEW_SOURCE
    netadapterview.cpp
)

add_executable(
    netadapterview
    ${NETADAPTERVIEW_SOURCE}
)

target_link_libraries( netadapterview
    hbm
)

SET(NETLINKVIEW_SOURCE
    netlinkview.cpp
)

add_executable(
    netlinkview
    ${NETLINKVIEW_SOURCE}
)

target_link_libraries( netlinkview
    hbm
)

SET(SERVER_SOURCE
    Server.cpp
)

add_executable(
    server
    ${SERVER_SOURCE}
)

target_link_libraries( server
    hbm
)

SET(CLIENT_SOURCE
    Client.cpp
)

add_executable(
    client
    ${CLIENT_SOURCE}
)

target_link_libraries( client
    hbm
)
