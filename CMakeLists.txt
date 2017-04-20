project("hbm" CXX)

cmake_minimum_required(VERSION 2.8)

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

