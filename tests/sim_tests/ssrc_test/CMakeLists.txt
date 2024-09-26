cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)


set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../)
project(ssrc_test)

if(NOT BUILD_NATIVE)
    set(APP_HW_TARGET XK-EVK-XU316)

    set(APP_COMPILER_FLAGS      -Os
                                -g
                                -Wall
                                -Wno-xcore-fptrgroup
                                -report
                                -fcmdline-buffer-bytes=2048
    )

    set(APP_DEPENDENT_MODULES "lib_src")

    set(APP_XC_SRCS src/dut/ssrc_test.xc)
    set(APP_C_SRCS "")
else()
    set(APP_COMPILER_FLAGS      -Os
                                -D__int64=int64_t
    )

    file(GLOB APP_SOURCES RELATIVE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/src/model/*.c)
    set(APP_XC_SRCS "")
    set(APP_C_SRCS      ${APP_SOURCES})
    set(APP_INCLUDES    src/model
                        ../../lib_src/src/multirate_hifi)
endif()

XMOS_REGISTER_APP()
