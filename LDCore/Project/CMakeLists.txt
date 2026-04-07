set(MODULE_NAME LDProject)
set(MODULE_TEST_NAME LDProjectTest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/Project/Project.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Project/ProjectSchema.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Project/ProjectContext.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Project/ProjectSettings.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Project/ProjectLoadAsync.h
)

set(MODULE_LIB
    Lib/Project.cpp
    Lib/ProjectSchemaKeys.h
    Lib/ProjectSchema.cpp
    Lib/ProjectContext.cpp
    Lib/ProjectSettings.cpp
    Lib/ProjectSettingsDefault.h
    Lib/ProjectLoadAsync.cpp
)

set(MODULE_TEST
    Test/ProjectTest.cpp
    Test/ProjectSchemaTest.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDAsset
    LDDataRegistry
    LDScene
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    LDProject
)
