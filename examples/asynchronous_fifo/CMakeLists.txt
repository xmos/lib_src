cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

set(APP_HW_TARGET XK-EVK-XU316)

set(APP_PCA_ENABLE ON)

set(APP_COMPILER_FLAGS  -O2
                        -g
                        -Wall
                        -report
)

set(APP_DEPENDENT_MODULES "lib_src")

set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../)

XMOS_REGISTER_APP()
