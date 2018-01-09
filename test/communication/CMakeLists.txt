project("hbm_communication_test")
cmake_minimum_required(VERSION 2.8)

include_directories(../../include)

#set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../../cmake)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp>=1.6.0)
if(JSONCPP_FOUND)
  include_directories( ${JSONCPP_INCLUDE_DIR} )
endif()

enable_testing()


SET(CMAKE_CXX_FLAGS "-std=c++11 -pthread -fno-common -Wall -Werror -Wshadow -Wextra -Winit-self -Wstrict-overflow=5 -Wunused-result -Wcast-qual -Wcast-align -Wpointer-arith -Wformat=2 -Wwrite-strings -Wmissing-declarations -pedantic")


#find_package(Boost ${QX_BOOST_VERSION}  COMPONENTS unit_test_framework)
find_package(Boost COMPONENTS unit_test_framework)
IF(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  LIST(APPEND ADDITIONAL_LIBS ${Boost_LIBRARIES})
endif(Boost_FOUND)

SET(MULTICASTSERVER_TEST
    multicastserver_test.cpp
)

add_executable(
    multicastserver.test
    ${MULTICASTSERVER_TEST}
)

target_link_libraries (
    multicastserver.test
    hbm
    ${JSONCPP_LIBRARIES}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

add_test(multicastserver_test multicastserver.test
--report_level=no
--log_level=all
#--output_format=xml
#--log_sink=${CMAKE_BINARY_DIR}/multicastserver_test.xml
)

SET(SOCKETNONBLOCKING_TEST
    socketnonblocking_test.cpp
)
set_source_files_properties(
    ${SOCKETNONBLOCKING_TEST}
    PROPERTIES COMPILE_FLAGS "-Wextra"
)

add_executable(
    socketnonblocking.test
    ${SOCKETNONBLOCKING_TEST}
)

target_link_libraries (
    socketnonblocking.test
    hbm
    ${JSONCPP_LIBRARIES}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

add_test(socketnonblocking_test socketnonblocking.test
--report_level=no
--log_level=all
#--output_format=xml
#--log_sink=${CMAKE_BINARY_DIR}/socketnonblocking_test.xml
)



SET(NETADAPTER_TEST
    netadapter_test.cpp
)
set_source_files_properties(
    ${NETADAPTER_TEST}
    PROPERTIES COMPILE_FLAGS "-Wextra"
)

add_executable(
    netadapter.test
    ${NETADAPTER_TEST}
)

target_link_libraries (
    netadapter.test
    hbm
    ${JSONCPP_LIBRARIES}
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

add_test(netadapter_test netadapter.test
--report_level=no
--log_level=all
#--output_format=xml
#--log_sink=${CMAKE_BINARY_DIR}/netadapter_test.xml
)
