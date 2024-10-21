cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(lib_src_examples)

add_subdirectory(app_asynchronous_fifo)
add_subdirectory(app_asrc_task)
