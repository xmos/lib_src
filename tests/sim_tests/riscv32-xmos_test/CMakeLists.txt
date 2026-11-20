cmake_minimum_required(VERSION 3.21)

# Older VX4 toolchains ship Clang but their XCommon CMake still selects xcc.
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
include(${CMAKE_CURRENT_LIST_DIR}/../../../examples/deps.cmake)

XMOS_REGISTER_APP()

# lib_logging 3.4.0 still includes <print.h>; VX4 gets it from lib_basix.
set_property(
    SOURCE "${XMOS_SANDBOX_DIR}/lib_logging/lib_logging/src/debug_printf.c"
    APPEND
    PROPERTY INCLUDE_DIRECTORIES "${XMOS_SANDBOX_DIR}/lib_basix/lib_basix/api/basix"
)

# Match the other VX4 apps: suppress these two warnings only in lib_logging.
set_property(
    SOURCE "${XMOS_SANDBOX_DIR}/lib_logging/lib_logging/src/debug_printf.c"
    APPEND
    PROPERTY COMPILE_OPTIONS -Wno-char-subscripts -Wno-unused-but-set-variable
)
