project("hbm" CXX)

cmake_minimum_required(VERSION 2.8)

SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
INCLUDE(generate_doxygen)

option(FEATURE_TOOLS                "Compile client software" OFF)
option(FEATURE_POST_BUILD_UNITTEST  "Automatically run unit-tests as a post build step" OFF)
option(FEATURE_GENERATE_DOC         "Generate documentation" OFF)
option(FEATURE_GCOV                 "Do test coverage analysis using gcov" OFF)
option(FEATURE_HBM_HARDWARE         "Building for hardware target" ON)

if(FEATURE_HBM_HARDWARE)
  add_definitions(-D_HBM_HARDWARE)
else()
  add_definitions(-D_STANDARD_HARDWARE)
endif()


add_subdirectory("lib")

if(FEATURE_TOOLS)
  add_subdirectory("tool/communication")
  add_subdirectory("tool/sys")
endif()

if(FEATURE_POST_BUILD_UNITTEST)
  enable_testing()
  add_subdirectory("test")
endif()
