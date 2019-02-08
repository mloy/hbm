project("hbm_communication_tool")
cmake_minimum_required(VERSION 2.8)

#set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../../cmake)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp>=1.6.0)
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
    ${JSONCPP_LIBRARIES}
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
    ${JSONCPP_LIBRARIES}
)

SET(SERVER_SOURCE
    Server.cpp
)

add_executable(
    server.bin
    ${SERVER_SOURCE}
)

target_link_libraries( server.bin
    hbm
    ${JSONCPP_LIBRARIES}
)

SET(CLIENT_SOURCE
    Client.cpp
)

add_executable(
    client.bin
    ${CLIENT_SOURCE}
)

target_link_libraries( client.bin
    hbm
    ${JSONCPP_LIBRARIES}
)
