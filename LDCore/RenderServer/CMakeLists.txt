set(MODULE_NAME LDRenderServer)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/RenderServer/RenderServer.h
    ${LUDENS_INCLUDE_DIR}/Ludens/RenderServer/ScreenLayer.h
)

set(MODULE_LIB
    Lib/RenderServer.cpp
    Lib/ScreenLayer.cpp
    Lib/ScreenRenderer.h
    Lib/ScreenRenderer.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDRenderBackend
    LDRenderGraph
    LDRenderComponent
)
