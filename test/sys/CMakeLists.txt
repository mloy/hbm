project("hbm_sys_test")
cmake_minimum_required(VERSION 2.8)

include_directories(../../include)

#set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../../cmake)


SET(CMAKE_CXX_FLAGS "-std=c++11 -pipe -fno-common -pthread -Wall -Werror -Wshadow -Wextra -Winit-self -Wstrict-overflow=5 -Wunused-result -Wcast-qual -Wcast-align -Wpointer-arith -Wformat=2 -Wwrite-strings -Wmissing-declarations -pedantic")

find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp>=1.6.0)
if(JSONCPP_FOUND)
  include_directories( ${JSONCPP_INCLUDE_DIR} )
endif()

#find_package(Boost ${QX_BOOST_VERSION}  COMPONENTS unit_test_framework)
find_package(Boost COMPONENTS unit_test_framework)
IF(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  LIST(APPEND ADDITIONAL_LIBS ${Boost_LIBRARIES})
endif(Boost_FOUND)


enable_testing()


SET(EXECUTECOMMAND_TEST
	executecommand_test.cpp
)

add_executable(
	executecommand.test
        ${EXECUTECOMMAND_TEST}
)

target_link_libraries (
	executecommand.test
        hbm
        ${JSONCPP_LIBRARIES}
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

#add_test(executecommand_test executecommand.test
#--report_level=no
#--log_level=all
#--output_format=xml
#--log_sink=${CMAKE_BINARY_DIR}/executecommand_test.xml)


SET(EVENTLOOP_TEST
	eventloop_test.cpp
)

add_executable(
	eventloop.test
	${EVENTLOOP_TEST}
)

target_link_libraries (
	eventloop.test
        hbm
        ${JSONCPP_LIBRARIES}
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

add_test(eventloop_test eventloop.test
--report_level=no
--log_level=all
#--output_format=xml
#--log_sink=${CMAKE_BINARY_DIR}/eventloop_test.xml
)
