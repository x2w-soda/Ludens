set(MODULE_NAME LDBAssetUtil)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensBuilder/BAssetUtil/BAssetUtil.h
)

set(MODULE_LIB
	Lib/BAssetUtil.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
	LDCoreLibs
)