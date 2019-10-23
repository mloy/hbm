project("hbm_string_test" CXX)

cmake_minimum_required(VERSION 2.8)




SET(WARN_FLAGS "-fno-common -Wall -Werror -Wshadow -Wextra -Winit-self -Wstrict-overflow=5 -Wunused-result -Wcast-qual -Wcast-align -Wpointer-arith -Wformat=2 -Wwrite-strings -Wmissing-declarations -pedantic")

SET(CMAKE_CXX_FLAGS "-std=c++11 -pipe ${WARN_FLAGS} ${CMAKE_COVERAGE_FLAGS}")

IF(FEATURE_GCOV)
  SET(CMAKE_CXX_FLAGS "-ftest-coverage -fprofile-arcs ${CMAKE_CXX_FLAGS}")
  SET(EXTRA_LINK_FLAGS "-fprofile-arcs")
ENDIF(FEATURE_GCOV)


find_package(Boost COMPONENTS unit_test_framework)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  list(APPEND ADDITIONAL_LIBS ${Boost_LIBRARIES})
endif(Boost_FOUND)



include_directories(../../include)

set(SOURCES_SPLITTEST
    splittest.cpp
)

add_executable( splittest ${SOURCES_SPLITTEST} )

set_target_properties(
  splittest
  PROPERTIES
  LINK_FLAGS "${EXTRA_LINK_FLAGS}"
)


target_link_libraries( splittest
    hbm
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

enable_testing()
add_test(splittest splittest
    --report_level=no
    --log_level=all
#    --output_format=xml
#    --log_sink=${CMAKE_BINARY_DIR}/splittest.xml
)

set(SOURCES_REPLACETEST
    replacetest.cpp
)

add_executable( replacetest ${SOURCES_REPLACETEST} )

set_target_properties(
  replacetest
  PROPERTIES
  LINK_FLAGS "${EXTRA_LINK_FLAGS}"
)


target_link_libraries( replacetest
    hbm
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
#    gcov
)

enable_testing()
add_test(replacetest replacetest
    --report_level=no
    --log_level=all
#    --output_format=xml
#    --log_sink=${CMAKE_BINARY_DIR}/replacetest.xml
)

set(SOURCES_TRIMTEST
    trimtest.cpp
)

add_executable( trimtest ${SOURCES_TRIMTEST} )

set_target_properties(
  trimtest
  PROPERTIES
  LINK_FLAGS "${EXTRA_LINK_FLAGS}"
)


target_link_libraries( trimtest
    hbm
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
#    gcov
)

enable_testing()
add_test(trimtest trimtest
    --report_level=no
    --log_level=all
#    --output_format=xml
#    --log_sink=${CMAKE_BINARY_DIR}/trimtest.xml
)
