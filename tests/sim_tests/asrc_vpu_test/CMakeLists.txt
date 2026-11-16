cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)

if(NOT BUILD_NATIVE)
    project(asrc_vpu_test)

    if(NOT DEFINED APP_HW_TARGET)
        set(APP_HW_TARGET XK-EVK-XU316)
    endif()

    if(DEFINED APP_HW_TARGET_VX OR APP_HW_TARGET STREQUAL "XK-EVK-XU416")
        set(APP_COMPILER_FLAGS -O2)

        set(APP_DEPENDENT_MODULES "lib_logging(3.4.0)" "lib_src")

        set(APP_INCLUDES
            "${CMAKE_CURRENT_LIST_DIR}/src"
        )

        get_filename_component(
            XMOS_SANDBOX_DIR
            "${CMAKE_CURRENT_LIST_DIR}/../../../../"
            ABSOLUTE
        )

        XMOS_REGISTER_APP()
        set_property(
            SOURCE "${XMOS_SANDBOX_DIR}/lib_logging/lib_logging/src/debug_printf.c"
            APPEND
            PROPERTY COMPILE_OPTIONS -Wno-char-subscripts -Wno-unused-but-set-variable
        )
    else()
        set(APP_PCA_ENABLE ON)
        set(APP_COMPILER_FLAGS -O2 -fxscope)

        include("${CMAKE_CURRENT_LIST_DIR}/../../../examples/deps.cmake")

        set(XMOS_SANDBOX_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../")

        XMOS_REGISTER_APP()
    endif()
endif()
