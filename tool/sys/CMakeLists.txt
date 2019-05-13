project("hbm_sys_tool")
cmake_minimum_required(VERSION 2.8)

#set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../../cmake)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp>=1.6.0)
if(JSONCPP_FOUND)
  include_directories( ${JSONCPP_INCLUDE_DIR} )
endif()

include_directories(../../include)

SET(CMAKE_CXX_FLAGS "-std=c++11 -fno-common -Wall -Werror -Wshadow -Wextra -Winit-self -Wstrict-overflow=5 -Wunused-result -Wcast-qual -Wcast-align -Wpointer-arith -Wformat=2 -Wwrite-strings -Wmissing-declarations -pedantic")


SET(EVENTLOOPPERFORMANCE_SOURCE
    eventloopperformance.cpp
)

add_executable(
    eventloopperformance
    ${EVENTLOOPPERFORMANCE_SOURCE}
)

target_link_libraries( eventloopperformance
    hbm
)
