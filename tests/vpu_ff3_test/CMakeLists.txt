
cmake_minimum_required(VERSION 3.21)

project(src_ff3_test)
enable_language(CXX C ASM)

option(test_src_96t_build "96 taps build" ON)
option(test_src_72t_build "72 taps build" ON)

if(NOT test_src_96t_build AND NOT test_src_72t_build)
    message(FATAL_ERROR "Build type must be specified")
endif()


if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS3A)
    if(test_src_96t_build)
        set(TARGET_NAME src_vpu_96t_test)

        add_executable(${TARGET_NAME})
        
        target_sources(${TARGET_NAME}
            PRIVATE
                fileio_app.c
                # build/test_src_96t_coefs.c
        )

        target_compile_definitions(${TARGET_NAME}
            PRIVATE
                "NUM_TAPS=96"
        )
    endif()

    if(test_src_72t_build)
        set(TARGET_NAME src_vpu_72t_test)

        add_executable(${TARGET_NAME})
        
        target_sources(${TARGET_NAME}
            PRIVATE
                fileio_app.c
                # build/test_src_72t_coefs.c
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