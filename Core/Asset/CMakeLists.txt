set(MODULE_NAME LDAsset)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/Asset.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/AssetManager.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/MeshAsset.h
	${LUDENS_INCLUDE_DIR}/Ludens/Asset/TextureAsset.h
)

set(MODULE_LIB
	Lib/AssetObj.h
	Lib/AssetManager.cpp
	Lib/MeshAsset.cpp
	Lib/TextureAsset.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDLog
	LDProfiler
	LDMedia
	LDRenderComponent
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
)
