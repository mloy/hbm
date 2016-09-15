project("hbm_string_test" CXX)

cmake_minimum_required(VERSION 2.8)


SET(CMAKE_CXX_FLAGS "-std=c++11 -pthread -fno-common -Wall -Werror -Wshadow -Wextra -Winit-self -Wstrict-overflow=5 -Wunused-result -Wcast-qual -Wcast-align -Wpointer-arith -Wformat=2 -Wwrite-strings -Wmissing-declarations -pedantic")

option(WITH_CPPCHECK "CppCheck" OFF)

if(WITH_CPPCHECK)
  find_package(cppcheck)
  include(CppcheckTargets)
endif()

message(STATUS "-----------------------------------------------------------")
message(STATUS "CppCheck            : " ${WITH_CPPCHECK})
message(STATUS "-----------------------------------------------------------")



find_package(Boost COMPONENTS unit_test_framework)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  list(APPEND ADDITIONAL_LIBS ${Boost_LIBRARIES})
endif(Boost_FOUND)


# enable coverage inspection:
# used by: /devscan/client/lib/CMakeLists.txt
# compile: --coverage
# link: gcov
option(GCOV_PACKAGE "GCOV" ON)


#include_directories(..)
include_directories(../../include)



#add_definitions("--coverage")


set(SOURCES_SPLITTEST
    splittest.cpp
#    ../split.cpp
)

add_executable( splittest ${SOURCES_SPLITTEST} )

target_link_libraries( splittest
    hbm
    jsoncpp
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    gcov
)

enable_testing()
add_test(splittest splittest
    --report_level=no
    --log_level=all
    --output_format=xml
    --log_sink=${CMAKE_BINARY_DIR}/splittest.xml
)

set(SOURCES_REPLACETEST
    replacetest.cpp
#    ../replace.cpp
)

add_executable( replacetest ${SOURCES_REPLACETEST} )

target_link_libraries( replacetest
    hbm
    jsoncpp
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    gcov
)

enable_testing()
add_test(replacetest replacetest
    --report_level=no
    --log_level=all
    --output_format=xml
    --log_sink=${CMAKE_BINARY_DIR}/replacetest.xml
)

set(SOURCES_TRIMTEST
    trimtest.cpp
#    ../trim.cpp
)

add_executable( trimtest ${SOURCES_TRIMTEST} )

target_link_libraries( trimtest
    hbm
    jsoncpp
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    gcov
)

enable_testing()
add_test(trimtest trimtest
    --report_level=no
    --log_level=all
    --output_format=xml
    --log_sink=${CMAKE_BINARY_DIR}/trimtest.xml
)
