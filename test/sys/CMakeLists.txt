project("hbm_sys_test")
cmake_minimum_required(VERSION 2.8)

include_directories(../../include)



SET(CMAKE_CXX_FLAGS "-std=c++14 -pipe -fno-common -pthread -Wall -Werror -Wshadow -Wextra -Winit-self -Wstrict-overflow=5 -Wunused-result -Wcast-qual -Wcast-align -Wpointer-arith -Wformat=2 -Wwrite-strings -Wmissing-declarations -pedantic")


IF(FEATURE_GCOV)
  SET(CMAKE_CXX_FLAGS "-ftest-coverage -fprofile-arcs ${CMAKE_CXX_FLAGS}")
  SET(EXTRA_LINK_FLAGS "-fprofile-arcs")
ENDIF(FEATURE_GCOV)


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

set_target_properties(
  executecommand.test
  PROPERTIES
  LINK_FLAGS "${EXTRA_LINK_FLAGS}"
)


target_link_libraries (
  executecommand.test
  hbm
  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

add_test(executecommand_test executecommand.test
--report_level=no
--log_level=all
#--output_format=xml
#--log_sink=${CMAKE_BINARY_DIR}/executecommand_test.xml
)


SET(EVENTLOOP_TEST
  eventloop_test.cpp
)

add_executable(
  eventloop.test
  ${EVENTLOOP_TEST}
)

set_target_properties(
  eventloop.test
  PROPERTIES
  LINK_FLAGS "${EXTRA_LINK_FLAGS}"
)


target_link_libraries (
	eventloop.test
        hbm
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

add_test(eventloop_test eventloop.test
--report_level=no
--log_level=all
#--output_format=xml
#--log_sink=${CMAKE_BINARY_DIR}/eventloop_test.xml
)
