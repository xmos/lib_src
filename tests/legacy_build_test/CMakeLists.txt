cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(legacy_build_test)

set(APP_HW_TARGET XK-EVK-XU316)

set(APP_PCA_ENABLE ON)

set(APP_COMPILER_FLAGS  -O2
                        -g
                        -report
                        -Wall
                        -Wno-xcore-fptrgroup
                        -fcmdline-buffer-bytes=512
)

set(APP_DEPENDENT_MODULES "lib_src")

set(APP_INCLUDES src)

set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../)

XMOS_REGISTER_APP()
