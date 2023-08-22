
cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../../../../xmos_cmake_toolchain/xs3a.cmake)

project(src_ff3_test)
enable_language(CXX C ASM)

option(test_src_96t_build "96 taps build" OFF)
option(test_src_72t_build "72 taps build" OFF)

if(NOT test_src_96t_build AND NOT test_src_72t_build)
    message(FATAL_ERROR "Build type must be specified")
endif()


if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS3A)

    add_subdirectory(../.. module_src)

if(test_src_96t_build)
    set(TARGET_NAME test_src_96t)

    add_executable(${TARGET_NAME})
    
    target_sources(${TARGET_NAME}
        PRIVATE
            fileio_app.c
            build/test_src_96t_coefs.c
    )

    target_compile_definitions(${TARGET_NAME}
        PRIVATE
            "NUM_TAPS=96"
    )


endif()

if(test_src_72t_build)
    set(TARGET_NAME test_src_72t)

    add_executable(${TARGET_NAME})
    
    target_sources(${TARGET_NAME}
        PRIVATE
            fileio_app.c
            build/test_src_72t_coefs.c
    )

    target_compile_definitions(${TARGET_NAME}
        PRIVATE
            "NUM_TAPS=72"
    )

endif()

    target_include_directories(${TARGET_NAME}
        PUBLIC
            build
    )

    target_link_libraries(${TARGET_NAME}
        PUBLIC
            fwk_xvf::src
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

endif()
