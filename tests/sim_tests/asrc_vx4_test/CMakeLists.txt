cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

if(NOT BUILD_NATIVE)
project(asrc_vx4_test)

set(APP_HW_TARGET XK-EVK-XU416)

set(APP_COMPILER_FLAGS  -O2
                        -fxscope
)

set(APP_DEPENDENT_MODULES lib_basix lib_logging)

set(LIB_SRC_DIR ../../../lib_src)
set(LIB_SRC_ABS ${CMAKE_CURRENT_LIST_DIR}/${LIB_SRC_DIR})

set(APP_INCLUDES
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${LIB_SRC_ABS}/api
    ${LIB_SRC_ABS}/src/fixed_factor_of_3
    ${LIB_SRC_ABS}/src/fixed_factor_of_3/ds3
    ${LIB_SRC_ABS}/src/fixed_factor_of_3/os3
    ${LIB_SRC_ABS}/src/fixed_factor_of_3_voice
    ${LIB_SRC_ABS}/src/multirate_hifi
    ${LIB_SRC_ABS}/src/multirate_hifi/asrc
    ${LIB_SRC_ABS}/src/multirate_hifi/ssrc
)

set(APP_C_SRCS
    ../asrc_vpu_test/src/test_asrc_vpu.c
    src/asrc_vx4_asm.c
    ${LIB_SRC_DIR}/src/multirate_hifi/asrc/src_mrhf_asrc.c
    ${LIB_SRC_DIR}/src/multirate_hifi/asrc/src_mrhf_asrc_wrapper.c
    ${LIB_SRC_DIR}/src/multirate_hifi/src_mrhf_filter_defs.c
    ${LIB_SRC_DIR}/src/multirate_hifi/src_mrhf_fir.c
    ${LIB_SRC_DIR}/src/multirate_hifi/src_mrhf_int_arithmetic.c
)

set(APP_ASM_SRCS
    src/asrc_vx4_inner_loops.S
)

get_filename_component(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../ ABSOLUTE)

XMOS_REGISTER_APP()
set_property(
    SOURCE ${XMOS_SANDBOX_DIR}/lib_logging/lib_logging/src/debug_printf.c
    APPEND PROPERTY COMPILE_OPTIONS -Wno-char-subscripts -Wno-unused-but-set-variable
)
endif()
