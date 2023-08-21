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

set(APP_LINK_OPTIONS
    -report
    -target=XCORE-AI-EXPLORER
    # Jenkins paths can get quite long
    -fcmdline-buffer-bytes=2048
)

set(APP_COMMON_LINK_LIBRARIES
    lib_src    
)


#*************************
# Create Target - Note just single tile as no RTOS
#*************************
set(TARGET_NAME ssrc_test)
add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL )
target_sources(${TARGET_NAME} PUBLIC ${APP_SOURCES})
target_include_directories(${TARGET_NAME} PUBLIC ${APP_INCLUDES})
target_compile_definitions(${TARGET_NAME} PUBLIC ${APP_COMPILE_DEFINITIONS})
target_compile_options(${TARGET_NAME} PRIVATE ${APP_COMPILER_FLAGS})
target_link_libraries(${TARGET_NAME} PUBLIC ${APP_COMMON_LINK_LIBRARIES})
target_link_options(${TARGET_NAME} PRIVATE ${APP_LINK_OPTIONS})
unset(TARGET_NAME)


# Copy output to a handy location
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.xe DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
