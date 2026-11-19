cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

if(NOT BUILD_NATIVE)
project(app_asrc_task)

if(NOT DEFINED APP_HW_TARGET)
    set(APP_HW_TARGET XK-EVK-XU316)
endif()

if(DEFINED APP_HW_TARGET_VX OR APP_HW_TARGET STREQUAL "XK-EVK-XU416")
    set(APP_PCA_ENABLE OFF)
else()
    set(APP_PCA_ENABLE ON)
endif()

set(APP_COMPILER_FLAGS  -Os
                        -g
                        -Wall
                        -DDEBUG_ASRC_TASK=1
                        -report
                        -fcmdline-buffer-bytes=16384
)

include(${CMAKE_CURRENT_LIST_DIR}/../deps.cmake)

set(APP_INCLUDES src)

set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../)
set(APP_XSCOPE_SRCS src/config.xscope)

XMOS_REGISTER_APP()

# Remove warnings from lib_logging https://github.com/xmos/lib_logging/issues/92
if(DEFINED APP_HW_TARGET_VX OR APP_HW_TARGET STREQUAL "XK-EVK-XU416")
    set_property(
        SOURCE "${XMOS_SANDBOX_DIR}/lib_logging/lib_logging/src/debug_printf.c"
        APPEND
        PROPERTY COMPILE_OPTIONS -Wno-char-subscripts -Wno-unused-but-set-variable
    )
endif()
endif()
