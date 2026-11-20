cmake_minimum_required(VERSION 3.21)

if("$ENV{XMOS_CMAKE_PATH}" STREQUAL "" OR
   NOT EXISTS "$ENV{XMOS_CMAKE_PATH}/xcore_riscv.cmake")
    message(FATAL_ERROR
        "Source a VX4 toolchain with RISC-V XCommon CMake support (tested with build 21).")
endif()

set(XMOS_TARGET_PROCESSOR RISCVXCORE)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(riscv32-xmos_test)

if(NOT XMOS_IS_RV)
    message(FATAL_ERROR "Source a VX4 toolchain with RISC-V XCommon CMake support.")
endif()

set(APP_COMPILER_FLAGS -Os -g -D__VX4B__)
set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../..)
# The VX4 RISC-V toolchain does not ship the legacy print.h and xccompat.h headers.
set(APP_INCLUDES ${XMOS_SANDBOX_DIR}/lib_basix/lib_basix/api/basix)
include(${CMAKE_CURRENT_LIST_DIR}/../../../examples/deps.cmake)

XMOS_REGISTER_APP()
