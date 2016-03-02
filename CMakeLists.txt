project("hbm" CXX)

cmake_minimum_required(VERSION 2.8)

enable_testing()

add_subdirectory("string/test")
add_subdirectory("sys/test")
add_subdirectory("communication/test")
add_subdirectory("communication/tool")
