cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(app_ff3)

if(NOT DEFINED APP_HW_TARGET)
    set(APP_HW_TARGET XK-EVK-XU316)
endif()

if(DEFINED APP_HW_TARGET_VX OR APP_HW_TARGET STREQUAL "XK-EVK-XU416")
    message(FATAL_ERROR "app_ff3 supports XS2 and XS3 targets only")
endif()

set(APP_PCA_ENABLE ON)
set(APP_COMPILER_FLAGS -Os -g -Wall -report -fcmdline-buffer-bytes=256)
set(APP_C_SRCS src/main.c ../shared_utils/sine_generator.c ../shared_utils/wav_writer.c)
set(APP_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/../shared_utils)
include(${CMAKE_CURRENT_LIST_DIR}/../deps.cmake)
set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../)

XMOS_REGISTER_APP()
