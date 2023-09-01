
cmake_minimum_required(VERSION 3.21)

project(src_ff3_test)
enable_language(C ASM)

option(test_src_vpu_72t_build "72 taps build" OFF)
option(test_src_vpu_96t_build "96 taps build" OFF)

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS3A AND (${test_src_vpu_72t_build} OR ${test_src_vpu_96t_build}))
    if(test_src_vpu_72t_build)
        set(NUMTAPS 72)
    elseif(test_src_vpu_96t_build)
        set(NUMTAPS 96)
    endif()

    set(TARGET_NAME test_src_vpu_${NUMTAPS}t)

    add_executable(${TARGET_NAME})

    target_sources(${TARGET_NAME}
        PRIVATE
            fileio_app.c
            autogen/test_src_${NUMTAPS}t_coefs.c
    )

    target_compile_definitions(${TARGET_NAME}
        PRIVATE
            "NUM_TAPS=${NUMTAPS}"
    )

    target_include_directories(${TARGET_NAME}
        PUBLIC
            autogen
    )

    target_link_libraries(${TARGET_NAME}
        PUBLIC
            lib_src
    )

    target_compile_options(${TARGET_NAME}
        PUBLIC
            "-g"
            "-O3"
            "-mno-dual-issue"
    )

    target_link_options(${TARGET_NAME}
        PUBLIC
            "-target=XCORE-AI-EXPLORER"
    )
    unset(TARGET_NAME)
endif()