set(LIB_NAME asrc_c_emulator_lib)
    add_library(${LIB_NAME} STATIC)

    target_sources(${LIB_NAME}
    PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/model/src/ASRC_wrapper.c
        ${CMAKE_CURRENT_LIST_DIR}/model/src/ASRC.c
        ${CMAKE_CURRENT_LIST_DIR}/model/src/FilterDefs.c
        ${CMAKE_CURRENT_LIST_DIR}/model/src/FIR.c
        ${CMAKE_CURRENT_LIST_DIR}/model/src/IntArithmetic.c
    )

    target_include_directories(${LIB_NAME}
        PRIVATE
            ${CMAKE_CURRENT_LIST_DIR}/model/src
            ${CMAKE_CURRENT_LIST_DIR}/../../lib_src/src/multirate_hifi)

    target_include_directories(${LIB_NAME}
        PUBLIC
            ${CMAKE_CURRENT_LIST_DIR}/model/api
    )

    target_compile_definitions(${LIB_NAME}
        PUBLIC
        __int64=int64_t
        )

    target_compile_options(${LIB_NAME}
        PRIVATE
            -Os
            -g
            -fPIC
    )
