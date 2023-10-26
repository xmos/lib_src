cmake_minimum_required(VERSION 3.21.0)

project(lib_src LANGUAGES C ASM)

if(PROJECT_IS_TOP_LEVEL)
    include(FetchContent)
    FetchContent_Declare(
      fwk_core
      GIT_REPOSITORY https://github.com/xmos/fwk_core.git
      GIT_TAG        v1.0.2
    )
    FetchContent_MakeAvailable(fwk_core)
endif()

add_subdirectory(tests)

# XS3 only libraries that use the Vector Processing Unit
if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS3A) 
    ## Source files
    file(GLOB_RECURSE LIB_C_SOURCES_XS3     lib_src/src/fixed_factor_vpu_voice/*.c )
    file(GLOB_RECURSE LIB_XC_SOURCES_XS3    lib_src/src/fixed_factor_vpu_voice/*.xc)
    file(GLOB_RECURSE LIB_ASM_SOURCES_XS3   lib_src/src/fixed_factor_vpu_voice/*.S )

    set(ADDITIONAL_INCLUDE_DIRS_XS3         lib_src/src/fixed_factor_vpu_voice
    )
endif()


if((${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS3A) OR (${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS2A))
    ## Source files
    file(GLOB_RECURSE LIB_C_SOURCES     lib_src/src/fixed_factor_of_3/*.c
                                        lib_src/src/fixed_factor_of_3_voice/*.c
                                        lib_src/src/multirate_hifi/*.c
    )
    file(GLOB_RECURSE LIB_XC_SOURCES    lib_src/src/fixed_factor_of_3/*.xc
                                        lib_src/src/fixed_factor_of_3_voice/*.xc
                                        lib_src/src/multirate_hifi/*.xc
    )
    file(GLOB_RECURSE LIB_ASM_SOURCES   lib_src/src/fixed_factor_of_3/*.S
                                        lib_src/src/fixed_factor_of_3_voice/*.S
                                        lib_src/src/multirate_hifi/*.S
    )
    
    ## Create library target
    add_library(lib_src STATIC EXCLUDE_FROM_ALL     ${LIB_C_SOURCES}
                                                    ${LIB_C_SOURCES_XS3}
                                                    ${LIB_ASM_SOURCES}
                                                    ${LIB_XC_SOURCES_XS3}
                                                    ${LIB_XC_SOURCES}
                                                    ${LIB_ASM_SOURCES_XS3}
    )
    
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
            ${ADDITIONAL_INCLUDE_DIRS_XS3}
    )

    target_link_libraries(lib_src
        PUBLIC
            # defined in fwk_core
            framework_core_legacy_compat
    )
    
    target_compile_options(lib_src
        PRIVATE 
            -O3
            -g
    )
endif()
