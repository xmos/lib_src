cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(lib_src_tests)

add_subdirectory(sim_tests)
add_subdirectory(hw_tests)
