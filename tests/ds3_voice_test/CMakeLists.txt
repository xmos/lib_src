
cmake_minimum_required(VERSION 3.21)

project(test_ds3_voice)
enable_language(CXX C ASM)

if((${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS3A) OR (${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS2A))
    set(TARGET_NAME test_ds3_voice)

    add_executable(${TARGET_NAME})
    
    target_sources(${TARGET_NAME}
        PRIVATE
            app_ds3_voice.xc
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
            "-target=XCORE-200-EXPLORER"
            "-report"
    )

endif()
