cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../)

project(asrc_test)

if(NOT BUILD_NATIVE) # dut
if(NOT DEFINED APP_HW_TARGET)
    set(APP_HW_TARGET XK-EVK-XU316)
endif()
include(${CMAKE_CURRENT_LIST_DIR}/../../../examples/deps.cmake)

set(ASRC_COMPILER_FLAGS_COMMON -Os
                            -g
                            -Wall
                            -report
                            -fcmdline-buffer-bytes=2048
)

set(APP_COMPILER_FLAGS_scalar ${ASRC_COMPILER_FLAGS_COMMON} -DSRC_USE_VPU=0)
set(APP_COMPILER_FLAGS_vpu ${ASRC_COMPILER_FLAGS_COMMON} -DSRC_USE_VPU=1)

set(APP_XC_SRCS "")
set(APP_C_SRCS src/dut/asrc_test.c)
set(APP_INCLUDES    src/dut)

else()  # golden reference

set(APP_COMPILER_FLAGS      -Os
                            -D__int64=int64_t
)
file(GLOB_RECURSE APP_SOURCES RELATIVE ${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/src/model/*.c)
set(APP_XC_SRCS "")
set(APP_C_SRCS      ${APP_SOURCES})
set(APP_INCLUDES    src/model
                    src/model/api
                    src/model/src
                    ../../../lib_src/src/multirate_hifi)
set(APP_XSCOPE_SRCS "")
endif()

XMOS_REGISTER_APP()

if(NOT BUILD_NATIVE AND (DEFINED APP_HW_TARGET_VX OR APP_HW_TARGET STREQUAL "XK-EVK-XU416"))
    set_property(
        SOURCE "${XMOS_SANDBOX_DIR}/lib_logging/lib_logging/src/debug_printf.c"
        APPEND
        PROPERTY COMPILE_OPTIONS -Wno-char-subscripts -Wno-unused-but-set-variable
    )
endif()

