project("hbm_communication_tool")
cmake_minimum_required(VERSION 2.8)

include_directories(../../..)


#enable_testing()

#set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../../../cmake)

SET(CMAKE_CXX_FLAGS "-std=c++0x -pipe -fno-common -ggdb -pthread")

SET(NETLINKVIEW_SOURCE
    ../linux/netadapter.cpp
    ../linux/netadapterlist.cpp
    ../linux/netlink.cpp
    ../../sys/linux/eventloop.cpp
    netlinkview.cpp
)
set_source_files_properties(
    ${NETLINKVIEW_SOURCE}
    PROPERTIES COMPILE_FLAGS "-Wextra -Winit-self -Wpointer-arith -Wformat=2 -Wcast-align -Wmissing-declarations -Werror"
)

add_executable(
    netlinkview.bin
    ${NETLINKVIEW_SOURCE}
)
