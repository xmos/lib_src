
cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../../../../xmos_cmake_toolchain/xs3a.cmake)

project(src_rat_test)
enable_language(CXX C ASM)

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS3A)

    add_subdirectory(../.. module_src)

    set(TARGET_NAME test_src_rat)

    add_executable(${TARGET_NAME})
    
    target_sources(${TARGET_NAME}
        PRIVATE
            fileio_app.c
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
