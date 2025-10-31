set(MODULE_NAME LDAsset)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/Asset.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/AssetManager.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/AssetSchema.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/AssetType/MeshAsset.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/AssetType/TextureAsset.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/AssetType/LuaScriptAsset.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/AssetType/AudioClipAsset.h
)

set(MODULE_LIB
	Lib/AssetObj.h
	Lib/AssetManager.cpp
	Lib/AssetWatcher.h
	Lib/AssetWatcher.cpp
	Lib/AssetSchema.cpp
	Lib/AssetType/MeshAsset.cpp
	Lib/AssetType/TextureAsset.cpp
	Lib/AssetType/LuaScriptAsset.cpp
	Lib/AssetType/AudioClipAsset.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDLog
	LDDSP
	LDProfiler
	LDMedia
	LDRenderComponent
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
)
