cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

if(NOT BUILD_NATIVE)
project(asrc_vpu_test)

if(NOT DEFINED APP_HW_TARGET)
    set(APP_HW_TARGET XK-EVK-XU316)
endif()

if(DEFINED APP_HW_TARGET_VX OR APP_HW_TARGET STREQUAL "XK-EVK-XU416")
    set(APP_COMPILER_FLAGS -O2)

    set(APP_DEPENDENT_MODULES "lib_basix(0.1.0)" "lib_logging(3.4.0)")

    set(LIB_SRC_DIR ../../../lib_src)
    set(LIB_SRC_ABS ${CMAKE_CURRENT_LIST_DIR}/${LIB_SRC_DIR})

    set(APP_INCLUDES
        ${CMAKE_CURRENT_LIST_DIR}/src/vx4
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
        src/test_asrc_vpu.c
        ${LIB_SRC_DIR}/src/multirate_hifi/asrc/src_mrhf_asrc.c
        ${LIB_SRC_DIR}/src/multirate_hifi/asrc/src_mrhf_asrc_wrapper.c
        ${LIB_SRC_DIR}/src/multirate_hifi/ssrc/src_mrhf_ssrc.c
        ${LIB_SRC_DIR}/src/multirate_hifi/ssrc/src_mrhf_ssrc_wrapper.c
        ${LIB_SRC_DIR}/src/multirate_hifi/src_mrhf_filter_defs.c
        ${LIB_SRC_DIR}/src/multirate_hifi/src_mrhf_fir.c
        ${LIB_SRC_DIR}/src/multirate_hifi/src_mrhf_int_arithmetic.c
        ${LIB_SRC_DIR}/src/multirate_hifi/src_mrhf_inner_loops_scalar_vx4b.c
        src/asrc_vx4_asm.c
    )

    set(APP_ASM_SRCS
        ${LIB_SRC_DIR}/src/multirate_hifi/src_mrhf_inner_loops_asm_vx4b.S
    )

    get_filename_component(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../ ABSOLUTE)

    XMOS_REGISTER_APP()
    set_property(
        SOURCE ${XMOS_SANDBOX_DIR}/lib_logging/lib_logging/src/debug_printf.c
        APPEND PROPERTY COMPILE_OPTIONS -Wno-char-subscripts -Wno-unused-but-set-variable
    )
else()
    set(APP_PCA_ENABLE ON)
    set(APP_COMPILER_FLAGS -O2 -fxscope)

    include(${CMAKE_CURRENT_LIST_DIR}/../../../examples/deps.cmake)

    set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../../)

    XMOS_REGISTER_APP()
endif()
endif()
