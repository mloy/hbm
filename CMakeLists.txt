project("hbm" CXX)

cmake_minimum_required(VERSION 2.8)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
INCLUDE(generate_doxygen)

option(FEATURE_TOOLS                "Compile client software" OFF)
option(FEATURE_POST_BUILD_UNITTEST  "Automatically run unit-tests as a post build step" OFF)

add_subdirectory("lib")

if(FEATURE_TOOLS)
  add_subdirectory("tool/communication")
endif()

if(FEATURE_POST_BUILD_UNITTEST)
  enable_testing()
  add_subdirectory("test")
endif()

ADD_CUSTOM_TARGET(doxygen ALL ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/generated/Doxyfile ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR} COMMENT "Generating API documentation with Doxygen" VERBATIM)
