# Test firmware
if ((${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS3A) OR (${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS2A))
    #**********************
    # Gather Sources
    #**********************

    file(GLOB APP_SOURCES               ${CMAKE_CURRENT_LIST_DIR}/*.c
                                        ${CMAKE_CURRENT_LIST_DIR}/*.xc
                                        ${CMAKE_CURRENT_LIST_DIR}/../../lib_src/src/asrc_task/*.c
    )

    set(APP_INCLUDES
        ${APP_SRC_PATH}/
        ${CMAKE_CURRENT_LIST_DIR}/
        ${CMAKE_CURRENT_LIST_DIR}/../../lib_src/src/asrc_task/
    )

    #**********************
    # Flags
    #**********************
    set(APP_COMPILER_FLAGS
        -Os
        -g
        -Wall
        -Wno-xcore-fptrgroup
        ${CMAKE_CURRENT_LIST_DIR}/config.xscope
    )

    set(APP_COMPILE_DEFINITIONS
        ASRC_TASK_CONFIG=1
        DEBUG_ASRC_TASK=1
    )

    set(ASRC_TASK_CONFIG_PATH "${CMAKE_CURRENT_LIST_DIR}")

    set(APP_COMMON_LINK_LIBRARIES
        lib_src
    )

    set(APP_LINK_OPTIONS
        -report
        ${CMAKE_CURRENT_LIST_DIR}/config.xscope
        -target=XCORE-AI-EXPLORER
        # Control paths can be quite long
        -fcmdline-buffer-bytes=16384
    )

    #*************************
    # Create Target - Note just single tile as no RTOS
    #*************************
    set(TARGET_NAME test_asrc_task)
    add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL )
    target_sources(${TARGET_NAME} PUBLIC ${APP_SOURCES})
    target_include_directories(${TARGET_NAME} PUBLIC ${APP_INCLUDES})
    target_compile_definitions(${TARGET_NAME} PUBLIC ${APP_COMPILE_DEFINITIONS})
    target_compile_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
    target_link_options(${TARGET_NAME} PRIVATE ${APP_LINK_OPTIONS})
    target_link_libraries(${TARGET_NAME} PUBLIC ${APP_COMMON_LINK_LIBRARIES})
    unset(TARGET_NAME)
endif()
