cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(asrc_task_test_ppm)

set(XMOS_SANDBOX_DIR    ${CMAKE_CURRENT_LIST_DIR}/..)

set(APP_HW_TARGET       XCORE-AI-EXPLORER)

set(APP_DEPENDENT_MODULES   lib_src)

set(APP_XSCOPE_SRCS src/config.xscope)

set(APP_COMPILER_FLAGS          -Os
                                -g
                                -report
                                -DASRC_TASK_CONFIG=1 # Enables user setting of ASRC configuration  
                                -DDEBUG_ASRC_TASK=1 # Prints cycle usage
                                -fcmdline-buffer-bytes=16384
                                )


# Needed by this application and not part of src library
set(APP_C_SRCS          ../../lib_src/src/asrc_task/asrc_task.c src/asrc_task_receive_samples.c)

set(APP_INCLUDES        src/
                        ../../lib_src/src/asrc_task/)

set(XMOS_SANDBOX_DIR ../../..)

XMOS_REGISTER_APP()
