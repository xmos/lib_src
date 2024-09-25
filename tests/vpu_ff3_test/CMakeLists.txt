cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(vpu_ff3_test)

set(APP_HW_TARGET XK-EVK-XU316)

set(APP_PCA_ENABLE ON)

set(XMOS_SANDBOX_DIR ${CMAKE_CURRENT_LIST_DIR}/../../../)

set(NUMTAPS 72 96)
set(NUM_PHASES 3)

foreach(taps ${NUMTAPS})
    math(EXPR taps_per_phase "${taps} / ${NUM_PHASES}" OUTPUT_FORMAT DECIMAL)
    set( AUTOGEN_DIR ${CMAKE_CURRENT_LIST_DIR}/src/src.autogen )
    file(RELATIVE_PATH REL_AUTOGEN_DIR ${CMAKE_CURRENT_LIST_DIR} ${AUTOGEN_DIR})

    set(AUTOGEN_C_FILE  ${AUTOGEN_DIR}/test_src_${taps}t_coefs.c)
    set(REL_AUTOGEN_C_FILE  ${REL_AUTOGEN_DIR}/test_src_${taps}t_coefs.c)

    set(AUTOGEN_H_FILE  ${AUTOGEN_DIR}/test_src_${taps}t_coefs.h)

    set(COEF_FILE_GEN_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/../../python/fixed_factor_vpu_voice/src_ff3_fir_gen.py)
    set(COEF_FILE_GEN_ARGS --output_dir ${AUTOGEN_DIR} --gen_c_files --num_taps_per_phase ${taps_per_phase} --num_phases ${NUM_PHASES} --name test_src_${taps}t)

    message(STATUS "args = ${COEF_FILE_GEN_ARGS}")

    add_custom_command(
        OUTPUT ${AUTOGEN_C_FILE}
        COMMAND python ${COEF_FILE_GEN_SCRIPT} ${COEF_FILE_GEN_ARGS}
        DEPENDS ${COEF_FILE_GEN_SCRIPT}
        COMMENT "Generate ff3 filter coefficients file"
        VERBATIM
    )

    add_custom_target(filter_coef_generation_${taps}t
                    DEPENDS ${AUTOGEN_C_FILE})


    set(APP_COMPILER_FLAGS_${taps}t     "-g"
                                        "-O3"
                                        "-mno-dual-issue"
                                        "-DNUM_TAPS=${taps}"
    )

    set(APP_DEPENDENT_MODULES "lib_src")

    set(APP_XC_SRCS "")
    set(APP_C_SRCS      src/fileio_app.c ${REL_AUTOGEN_C_FILE})
    set(APP_INCLUDES    ${REL_AUTOGEN_DIR})

    XMOS_REGISTER_APP()

    unset(APP_COMPILER_FLAGS_${taps}t)

    foreach(target ${APP_BUILD_TARGETS})
        add_dependencies(${target} filter_coef_generation_${taps}t)
    endforeach()

endforeach()

