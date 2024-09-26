cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

if(NOT BUILD_NATIVE)
set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../)

project(asrc_test_xscope_fileio)
set(APP_HW_TARGET XK-EVK-XU316)
set(APP_DEPENDENT_MODULES "lib_src" "xscope_fileio")

set(APP_COMPILER_FLAGS      -Os
                            -g
                            -Wall
                            -Wno-xcore-fptrgroup
                            -report
                            -fcmdline-buffer-bytes=2048
                            -DXSCOPE
                            -DTEST_WAV_XSCOPE=1
)

set(APP_XC_SRCS src/main.xc)
file(GLOB_RECURSE APP_SOURCES RELATIVE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/src/*.c)
set(APP_C_SRCS ${APP_SOURCES})
set(APP_INCLUDES    src/file_utils)
XMOS_REGISTER_APP()
endif()


