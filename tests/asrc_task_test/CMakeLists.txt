# Test firmware
if ((${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS3A) OR (${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS2A))
    #**********************
    # Gather Sources
    #**********************

    file(GLOB APP_SOURCES               ${CMAKE_CURRENT_LIST_DIR}/*.c
                                        ${CMAKE_CURRENT_LIST_DIR}/*.xc
                                        # ${CMAKE_CURRENT_LIST_DIR}/../../../lib_src/src/*.c
    )

    set(APP_INCLUDES
        ${APP_SRC_PATH}/
        ${CMAKE_CURRENT_LIST_DIR}/
    )
    message(STATUS APP_INCLUDES: ${APP_INCLUDES})
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

    set(ASRC_TASK_CONFIG_PATH "${CMAKE_CURRENT_LIST_DIR}")
    message(STATUS ASRC_TASK_CONFIG_PATH from app, ${ASRC_TASK_CONFIG_PATH})


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
