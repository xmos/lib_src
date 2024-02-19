# Test firmware
if ((${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS3A) OR (${CMAKE_SYSTEM_PROCESSOR} STREQUAL XCORE_XS2A))
    set(XSCOPE_FILEIO_PATH ${CMAKE_BINARY_DIR}/xscope_fileio)

    if(NOT EXISTS ${XSCOPE_FILEIO_PATH})
        message("Cloning xscope_fileio")
        execute_process(COMMAND git clone git@github.com:xmos/xscope_fileio ${XSCOPE_FILEIO_PATH} --recurse-submodules)
    endif()

    set(XSCOPE_FILEIO_APP fwk_voice_deps_xscope_fileio)
    add_library(${XSCOPE_FILEIO_APP}  INTERFACE)

    target_sources(${XSCOPE_FILEIO_APP}
        INTERFACE
            ${XSCOPE_FILEIO_PATH}/xscope_fileio/src/xscope_io_device.c)

    target_include_directories(${XSCOPE_FILEIO_APP}
        INTERFACE
            ${XSCOPE_FILEIO_PATH}/xscope_fileio
            ${XSCOPE_FILEIO_PATH}/xscope_fileio/api)

    target_compile_definitions(${XSCOPE_FILEIO_APP}
        INTERFACE
            TEST_WAV_XSCOPE=1)

    #**********************
    # Gather Sources
    #**********************

    set(APP_SOURCES_DAT  ${CMAKE_CURRENT_LIST_DIR}/dut/src/asrc_test.xc)

    set(APP_SOURCES_XSCOPE_FILEIO
            ${CMAKE_CURRENT_LIST_DIR}/dut/with_xscope_fileio/src/main.xc
            ${CMAKE_CURRENT_LIST_DIR}/dut/with_xscope_fileio/src/asrc_test.c
            ${CMAKE_CURRENT_LIST_DIR}/dut/with_xscope_fileio/src/file_utils/fileio.c
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
        -target=XK-EVK-XU316
        -DXSCOPE
    )

    set(APP_COMPILE_DEFINITIONS
    )

    set(APP_COMMON_LINK_LIBRARIES
        lib_src
    )

    set(APP_LINK_OPTIONS
        -report
        -target=XK-EVK-XU316
        # Jenkins paths can get quite long
        -fcmdline-buffer-bytes=2048
    )

    #*************************
    # Create Target - Note just single tile as no RTOS
    #*************************
    set(TARGET_NAME test_asrc)
    add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL )
    target_sources(${TARGET_NAME} PUBLIC ${APP_SOURCES_DAT})
    target_include_directories(${TARGET_NAME} PUBLIC ${APP_INCLUDES})
    target_compile_definitions(${TARGET_NAME} PUBLIC ${APP_COMPILE_DEFINITIONS})
    target_compile_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
    target_link_options(${TARGET_NAME} PRIVATE ${APP_LINK_OPTIONS})
    target_link_libraries(${TARGET_NAME} PUBLIC ${APP_COMMON_LINK_LIBRARIES})
    unset(TARGET_NAME)

    set(TARGET_NAME test_asrc_xscope_fileio)
    add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL )
    target_sources(${TARGET_NAME} PUBLIC ${APP_SOURCES_XSCOPE_FILEIO})
    target_include_directories(${TARGET_NAME} PUBLIC ${APP_INCLUDES} ${CMAKE_CURRENT_LIST_DIR}/dut/with_xscope_fileio/src/file_utils)
    target_compile_definitions(${TARGET_NAME} PUBLIC ${APP_COMPILE_DEFINITIONS})
    target_compile_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS} "${CMAKE_CURRENT_LIST_DIR}/dut/with_xscope_fileio/config.xscope")
    target_link_options(${TARGET_NAME} PRIVATE ${APP_LINK_OPTIONS} "${CMAKE_CURRENT_LIST_DIR}/dut/with_xscope_fileio/config.xscope")
    target_link_libraries(${TARGET_NAME} PUBLIC ${APP_COMMON_LINK_LIBRARIES} ${XSCOPE_FILEIO_APP})
    unset(TARGET_NAME)

else ()

# Golden ref app
    include(${CMAKE_CURRENT_LIST_DIR}/asrc_c_emulator.cmake)
    set(TARGET_NAME asrc_golden)
    project(${TARGET_NAME} C)

    file(GLOB APP_SOURCES               ${CMAKE_CURRENT_LIST_DIR}/model/Main.c
    )

    set(APP_INCLUDES
         ${CMAKE_CURRENT_LIST_DIR}/model
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
    target_link_libraries(${TARGET_NAME} PUBLIC asrc_c_emulator_lib)
    unset(TARGET_NAME)
endif()
