cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

if(NOT BUILD_NATIVE)
project(asrc_task_test)

set(APP_HW_TARGET XK-EVK-XU316)

set(APP_PCA_ENABLE ON)

set(APP_COMPILER_FLAGS  -Os
                        -g
                        -Wall
                        -Wno-xcore-fptrgroup
                        -DASRC_TASK_CONFIG=1
                        -DDEBUG_ASRC_TASK=1
                        -report
                        -fcmdline-buffer-bytes=16384
)

set(APP_DEPENDENT_MODULES "lib_src")

set(APP_INCLUDES src)

set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../)

XMOS_REGISTER_APP()
endif()
