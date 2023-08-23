
cmake_minimum_required(VERSION 3.21)

project(src_rat_test)
enable_language(CXX C ASM)

if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS3A)

    set(TARGET_NAME test_src_vpu_rat)

    add_executable(${TARGET_NAME})
    
    target_sources(${TARGET_NAME}
        PRIVATE
            fileio_app.c
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

endif()
