set(MODULE_NAME LDRenderGraph)
set(MODULE_TEST_NAME LDRenderGraphTest)

set(MODULE_INCLUDE
  "${LUDENS_INCLUDE_DIR}/Ludens/RenderGraph/RGraph.h"
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

add_library(${MODULE_NAME}
    ${MODULE_INCLUDE}
    ${MODULE_LIB}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_link_libraries(${MODULE_NAME} PUBLIC
    LDRenderBackend
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
)

add_executable(${MODULE_TEST_NAME}
    ${MODULE_TEST}
)

set_target_properties(${MODULE_TEST_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
)

target_include_directories(${MODULE_TEST_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_SOURCE_DIR}
)
