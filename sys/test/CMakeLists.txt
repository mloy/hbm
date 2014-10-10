project("hbm_sys_test")
cmake_minimum_required(VERSION 2.8)

include_directories(../../..)


set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../../../cmake)


find_package(Boost ${QX_BOOST_VERSION}  COMPONENTS unit_test_framework)

IF(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  LIST(APPEND ADDITIONAL_LIBS ${Boost_LIBRARIES})
endif(Boost_FOUND)

SET(EXECUTECOMMAND_TEST
	../executecommand.cpp
	executecommand_test.cpp
)
set_source_files_properties(
	${STRING_UTILS_TEST}
	PROPERTIES COMPILE_FLAGS "-Wextra"
)

add_executable(
	executecommand.test
	${EXECUTECOMMAND_TEST}
)

target_link_libraries (
	executecommand.test
	${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)


add_test(executecommand_test executecommand.test
--report_level=no
--log_level=all
--output_format=xml
--log_sink=${CMAKE_BINARY_DIR}/executecommand_test.xml)
