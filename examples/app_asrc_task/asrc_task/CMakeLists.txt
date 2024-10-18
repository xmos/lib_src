cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

if(NOT BUILD_NATIVE)
project(asrc_task)

set(APP_HW_TARGET XK-EVK-XU316)

set(APP_PCA_ENABLE ON)

set(APP_COMPILER_FLAGS  -Os
                        -g
                        -Wall
                        -Wno-xcore-fptrgroup
                        -DDEBUG_ASRC_TASK=1
                        -report
                        -fcmdline-buffer-bytes=16384
)

include(${CMAKE_CURRENT_LIST_DIR}/../deps.cmake)

set(APP_INCLUDES src)

set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../)

XMOS_REGISTER_APP()
endif()
