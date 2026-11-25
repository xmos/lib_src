cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(app_ssrc)

if(NOT DEFINED APP_HW_TARGET)
    set(APP_HW_TARGET XK-EVK-XU316)
endif()

if(DEFINED APP_HW_TARGET_VX OR APP_HW_TARGET STREQUAL "XK-EVK-XU416")
    set(APP_PCA_ENABLE OFF)
else()
    set(APP_PCA_ENABLE ON)
endif()

set(APP_COMPILER_FLAGS -Os -g -Wall -report -fcmdline-buffer-bytes=256)
set(APP_C_SRCS src/main.c ../shared_utils/sine_generator.c ../shared_utils/wav_writer.c)
set(APP_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/../shared_utils)
include(${CMAKE_CURRENT_LIST_DIR}/../deps.cmake)
set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../)

XMOS_REGISTER_APP()

if(DEFINED APP_HW_TARGET_VX OR APP_HW_TARGET STREQUAL "XK-EVK-XU416")
    set_property(
        SOURCE "${XMOS_SANDBOX_DIR}/lib_logging/lib_logging/src/debug_printf.c"
        APPEND
        PROPERTY COMPILE_OPTIONS -Wno-char-subscripts -Wno-unused-but-set-variable
    )
endif()
