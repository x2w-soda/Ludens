set(MODULE_NAME LDProject)
set(MODULE_TEST_NAME LDProjectTest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/Project/Project.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Project/ProjectSchema.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Project/ProjectSettings.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Project/ProjectSettingsDefault.h
)

set(MODULE_LIB
    Lib/Project.cpp
    Lib/ProjectSchemaKeys.h
    Lib/ProjectSchema.cpp
    Lib/ProjectSettings.cpp
)

set(MODULE_TEST
    Test/ProjectTest.cpp
    Test/ProjectSchemaTest.cpp
)

add_library(${MODULE_NAME}
    ${MODULE_LIB}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_link_libraries(${MODULE_NAME} PUBLIC
    LDAsset
    LDDataRegistry
    LDScene
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_EXTRA_DIR}
)

add_executable(${MODULE_TEST_NAME}
    ${MODULE_TEST}
)

set_target_properties(${MODULE_TEST_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_include_directories(${MODULE_TEST_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_SOURCE_DIR}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    LDProject
)
