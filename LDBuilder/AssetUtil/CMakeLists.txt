set(MODULE_NAME LDAssetUtil)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetUtil/AssetUtil.h
)

set(MODULE_LIB
	Lib/AssetUtil.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
	LDCoreLibs
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_BUILDER_MODULE_FOLDER})
