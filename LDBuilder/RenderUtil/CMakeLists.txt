set(MODULE_NAME LDRenderUtil)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensBuilder/RenderUtil/RenderUtil.h
)

set(MODULE_LIB
	Lib/RenderUtil.cpp
)

add_ludens_builder_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)