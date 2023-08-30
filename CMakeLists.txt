cmake_minimum_required(VERSION 3.21.0)
project(lib_src LANGUAGES C ASM)

if(PROJECT_IS_TOP_LEVEL)
    include(FetchContent)
    FetchContent_Declare(
      fwk_core
      GIT_REPOSITORY https://github.com/xmos/fwk_core.git
      GIT_TAG        v1.0.0
    )
    FetchContent_MakeAvailable(fwk_core)
endif()

if((${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS3A) OR (${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS2A))
    ## Source files
    file(GLOB_RECURSE LIB_C_SOURCES   lib_src/src/*.c )
    file(GLOB_RECURSE LIB_XC_SOURCES  lib_src/src/*.xc)
    file(GLOB_RECURSE LIB_ASM_SOURCES lib_src/src/*.S )

    ## Create library target
    add_library(lib_src STATIC EXCLUDE_FROM_ALL ${LIB_C_SOURCES} ${LIB_ASM_SOURCES} ${LIB_XC_SOURCES})
    target_include_directories(lib_src
        PUBLIC
            lib_src/api
            lib_src/src/fixed_factor_of_3
            lib_src/src/fixed_factor_of_3/ds3
            lib_src/src/fixed_factor_of_3/os3
            lib_src/src/fixed_factor_of_3_voice
            lib_src/src/fixed_factor_of_3_voice/ds3_voice
            lib_src/src/fixed_factor_of_3_voice/us3_voice
            lib_src/src/multirate_hifi
            lib_src/src/multirate_hifi/asrc
            lib_src/src/multirate_hifi/ssrc
    )
    target_link_libraries(lib_src
        PUBLIC
            # defined in fwk_core
            framework_core_legacy_compat
    )
    target_compile_options(lib_src
        PRIVATE 
            -O3
    )

endif()
