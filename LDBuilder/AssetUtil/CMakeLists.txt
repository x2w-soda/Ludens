set(MODULE_NAME LDAssetUtil)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetUtil/AssetUtil.h
)

set(MODULE_LIB
	Lib/AssetUtil.cpp
)

add_ludens_builder_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)