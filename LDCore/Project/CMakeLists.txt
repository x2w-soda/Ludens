set(MODULE_NAME LDProject)
set(MODULE_TEST_NAME LDProjectTest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/Project/Project.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Project/ProjectSchema.h
)

set(MODULE_LIB
    Lib/Project.cpp
    Lib/ProjectSchema.cpp
)

set(MODULE_TEST
    Test/ProjectTest.cpp
    Test/ProjectSchemaTest.cpp
)

add_library(${MODULE_NAME}
    ${MODULE_LIB}
)

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

target_include_directories(${MODULE_TEST_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_SOURCE_DIR}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    LDProject
)
