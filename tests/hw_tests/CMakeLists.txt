cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(lib_src_hw_tests)

add_subdirectory(asrc_task_test)
add_subdirectory(asrc_task_test_ppm)
add_subdirectory(asrc_test_xscope_fileio)
add_subdirectory(asynchronous_fifo_asrc_test)
