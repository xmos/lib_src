cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

if(NOT BUILD_NATIVE)
project(asrc_vpu_test)

set(APP_HW_TARGET XK-EVK-XU316)

set(APP_PCA_ENABLE ON)

set(APP_COMPILER_FLAGS  -O2
                        -fxscope
)

include(${CMAKE_CURRENT_LIST_DIR}/../../../examples/deps.cmake)

set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../)

XMOS_REGISTER_APP()
endif()
