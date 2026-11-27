set(LIB_NAME lib_src)
set(LIB_VERSION 2.7.1)

set(LIB_DEPENDENT_MODULES   "lib_logging(3.4.0)"
                            "lib_basix(0.1.1)")

set(LIB_COMPILER_FLAGS -Wno-missing-braces -O3 -Wall)

if(DEFINED APP_HW_TARGET_VX OR APP_HW_TARGET STREQUAL "XK-EVK-XU416")
    list(APPEND LIB_COMPILER_FLAGS -Wno-fptrgroup)
else()
    list(APPEND LIB_COMPILER_FLAGS -Wno-xcore-fptrgroup)
endif()

set(LIB_OPTIONAL_HEADERS src_conf.h asrc_task_config.h)

set(LIB_INCLUDES   api
                   src/fixed_factor_of_3_hifi
                   src/fixed_factor_of_3_hifi/ds3
                   src/fixed_factor_of_3_hifi/os3
                   src/fixed_factor_of_3_voice
                   src/multirate_hifi
                   src/multirate_hifi/asrc
                   src/multirate_hifi/ssrc
                   src/fixed_factor_vpu_voice
                   src/asrc_task)

if(DEFINED APP_HW_TARGET_VX OR APP_HW_TARGET STREQUAL "XK-EVK-XU416")
    set(LIB_XC_SRCS "")
    file(GLOB_RECURSE LIB_C_SRCS CONFIGURE_DEPENDS
         RELATIVE "${CMAKE_CURRENT_LIST_DIR}"
         "${CMAKE_CURRENT_LIST_DIR}/src/*.c")
    list(REMOVE_ITEM LIB_C_SRCS
         "src/fixed_factor_of_3_hifi/ds3/src_ff3_ds3.c"
         "src/fixed_factor_of_3_hifi/os3/src_ff3_os3.c")
endif()

XMOS_REGISTER_MODULE()

