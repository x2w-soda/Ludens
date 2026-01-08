set(MODULE_NAME LDAsset)
set(MODULE_TEST_NAME LDAssetTest)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/Asset.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/AssetManager.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/AssetRegistry.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/AssetSchema.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/Template/UITemplate.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/Template/UITemplateSchema.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/AssetType/BlobAsset.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/AssetType/FontAsset.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/AssetType/MeshAsset.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/AssetType/Texture2DAsset.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/AssetType/TextureCubeAsset.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/AssetType/LuaScriptAsset.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/AssetType/AudioClipAsset.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/AssetType/UITemplateAsset.h
)

set(MODULE_LIB
	Lib/AssetObj.h
	Lib/AssetManager.cpp
	Lib/AssetRegistry.cpp
	Lib/AssetWatcher.h
	Lib/AssetWatcher.cpp
	Lib/AssetSchemaKeys.h
	Lib/AssetSchema.cpp
	Lib/Template/UITemplate.cpp
	Lib/Template/UITemplateObj.h
	Lib/Template/UITemplateSchemaKeys.h
	Lib/Template/UITemplateSchema.cpp
	Lib/AssetType/BlobAssetObj.h
	Lib/AssetType/BlobAsset.cpp
	Lib/AssetType/FontAssetObj.h
	Lib/AssetType/FontAsset.cpp
	Lib/AssetType/MeshAssetObj.h
	Lib/AssetType/MeshAsset.cpp
	Lib/AssetType/Texture2DAssetObj.h
	Lib/AssetType/Texture2DAsset.cpp
	Lib/AssetType/TextureCubeAssetObj.h
	Lib/AssetType/TextureCubeAsset.cpp
	Lib/AssetType/LuaScriptAssetObj.h
	Lib/AssetType/LuaScriptAsset.cpp
	Lib/AssetType/AudioClipAssetObj.h
	Lib/AssetType/AudioClipAsset.cpp
	Lib/AssetType/UITemplateAssetObj.h
	Lib/AssetType/UITemplateAsset.cpp
)

set(MODULE_TEST
	Test/AssetTest.cpp
	Test/UITemplateTest.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_link_libraries(${MODULE_NAME} PRIVATE
	LDLog
	LDDSP
	LDProfiler
	LDMedia
	LDRenderComponent
	LDUI
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
)

add_executable(${MODULE_TEST_NAME}
    ${MODULE_TEST}
)

set_target_properties(${MODULE_TEST_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_include_directories(${MODULE_TEST_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_SOURCE_DIR}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
)