set(MODULE_NAME LDRenderGraph)
set(MODULE_TEST_NAME LDRenderGraphTest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/RenderGraph/RGraph.h
)

set(MODULE_LIB
    Lib/RGraph.cpp
    Lib/RGraphObj.h
    Lib/RComponent.h
    Lib/RComponent.cpp
)

set(MODULE_TEST
    Test/RGraphTest.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDRenderBackend
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
)