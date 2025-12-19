set(MODULE_NAME LDRenderServer)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/RenderServer/RenderServer.h
    ${LUDENS_INCLUDE_DIR}/Ludens/RenderServer/ScreenLayer.h
)

set(MODULE_LIB
    Lib/RenderServer.cpp
    Lib/ScreenLayer.cpp
)

add_library(${MODULE_NAME}
    ${MODULE_INCLUDE}
    ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDRenderBackend
    LDRenderGraph
    LDRenderComponent
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
)