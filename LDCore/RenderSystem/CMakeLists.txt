set(MODULE_NAME LDRenderSystem)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/RenderSystem/RenderSystem.h
)

set(MODULE_LIB
    Lib/RenderSystemObj.h
    Lib/RenderSystem.cpp
    Lib/ScreenLayer.h
    Lib/ScreenLayer.cpp
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
