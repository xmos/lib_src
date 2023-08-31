# Test firmware
if ((${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS3A) OR (${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS2A))
    #**********************
    # Gather Sources
    #**********************

    file(GLOB APP_SOURCES               ${CMAKE_CURRENT_LIST_DIR}/dut/*.c
                                        ${CMAKE_CURRENT_LIST_DIR}/dut/*.xc
    )

    set(APP_INCLUDES
        ${APP_SRC_PATH}/dut
        ${APP_SRC_PATH}/default_params
    )

    #**********************
    # Flags
    #**********************
    set(APP_COMPILER_FLAGS
        -Os
        -g
        -Wall
        -Wno-xcore-fptrgroup
        -target=XCORE-AI-EXPLORER
    )

    set(APP_COMPILE_DEFINITIONS
    )

    set(APP_COMMON_LINK_LIBRARIES
        lib_src    
    )

    set(APP_LINK_OPTIONS
        -report
        -target=XCORE-AI-EXPLORER
        # Jenkins paths can get quite long
        -fcmdline-buffer-bytes=2048
    )

    #*************************
    # Create Target - Note just single tile as no RTOS
    #*************************
    set(TARGET_NAME test_ssrc)
    add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL )
    target_sources(${TARGET_NAME} PUBLIC ${APP_SOURCES})
    target_include_directories(${TARGET_NAME} PUBLIC ${APP_INCLUDES})
    target_compile_definitions(${TARGET_NAME} PUBLIC ${APP_COMPILE_DEFINITIONS})
    target_compile_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
    target_link_options(${TARGET_NAME} PRIVATE ${APP_LINK_OPTIONS})
    target_link_libraries(${TARGET_NAME} PUBLIC ${APP_COMMON_LINK_LIBRARIES})

    unset(TARGET_NAME)

else ()

# Golden ref app
    set(TARGET_NAME ssrc_golden)
    project(${TARGET_NAME} C)

    file(GLOB APP_SOURCES               ${CMAKE_CURRENT_LIST_DIR}/model/*.c
    )

    set(APP_INCLUDES
         ${CMAKE_CURRENT_LIST_DIR}/../../lib_src/src/multirate_hifi
    )

    set(APP_COMPILER_FLAGS
        -Os
    )

    set(APP_COMPILE_DEFINITIONS
        __int64=int64_t
    )

    add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL)
    set_target_properties(${TARGET_NAME} PROPERTIES LINKER_LANGUAGE C)
    target_sources(${TARGET_NAME} PUBLIC ${APP_SOURCES})
    target_include_directories(${TARGET_NAME} PUBLIC ${APP_INCLUDES})
    target_compile_definitions(${TARGET_NAME} PUBLIC ${APP_COMPILE_DEFINITIONS})
    target_compile_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
    target_link_libraries(${TARGET_NAME} PUBLIC ${APP_COMMON_LINK_LIBRARIES})
    unset(TARGET_NAME)
endif()

