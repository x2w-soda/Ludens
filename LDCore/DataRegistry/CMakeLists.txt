set(MODULE_NAME LDDataRegistry)
set(MODULE_TEST_NAME LDDataRegistryTest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/DataRegistry/DataComponent.h
    ${LUDENS_INCLUDE_DIR}/Ludens/DataRegistry/DataRegistry.h
)

set(MODULE_LIB
    Lib/DataRegistry.cpp
)

set(MODULE_TEST
    Test/DataRegistryTest.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDSystem
    LDAudioServer
    LDRenderServer
    LDAsset
)

add_executable(${MODULE_TEST_NAME}
    ${MODULE_TEST}
)

set_target_properties(${MODULE_TEST_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_include_directories(${MODULE_TEST_NAME} PRIVATE
    ${LUDENS_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
)