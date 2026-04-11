set(MODULE_NAME LDDataRegistry)
set(MODULE_TEST_NAME LDDataRegistryTest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/DataRegistry/DataComponent.h
    ${LUDENS_INCLUDE_DIR}/Ludens/DataRegistry/DataRegistry.h
    ${LUDENS_INCLUDE_DIR}/Ludens/DataRegistry/DataRegistryDef.h
    ${LUDENS_INCLUDE_DIR}/Ludens/DataRegistry/TransformRegistry.h
)

set(MODULE_LIB
    Lib/DataRegistry.cpp
    Lib/TransformRegistry.cpp
)

set(MODULE_TEST
    Test/DataRegistryTest.cpp
    Test/TransformRegistryTest.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDSystem
    LDAudioSystem
    LDRenderSystem
    LDAsset
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
)