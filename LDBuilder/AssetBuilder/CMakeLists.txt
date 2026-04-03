set(MODULE_NAME LDAssetBuilder)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetImporter.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetImportInfo.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetType/AssetBuilders.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetType/BlobAssetBuilder.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetType/FontAssetBuilder.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetType/MeshAssetBuilder.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetType/UITemplateAssetBuilder.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetType/AudioClipAssetBuilder.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetType/Texture2DAssetBuilder.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetType/TextureCubeAssetBuilder.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetType/LuaScriptAssetBuilder.h
)

set(MODULE_LIB
    Lib/AssetImporter.cpp
    Lib/AssetImportJob.h
    Lib/AssetImportJob.cpp
    Lib/AssetBuilderMeta.h
    Lib/AssetBuilderMeta.cpp
    Lib/AssetType/BlobAssetBuilder.cpp
    Lib/AssetType/FontAssetBuilder.cpp
    Lib/AssetType/MeshAssetBuilder.cpp
    Lib/AssetType/UITemplateAssetBuilder.cpp
    Lib/AssetType/AudioClipAssetBuilder.cpp
    Lib/AssetType/Texture2DAssetBuilder.cpp
    Lib/AssetType/TextureCubeAssetBuilder.cpp
    Lib/AssetType/LuaScriptAssetBuilder.cpp
)

add_ludens_builder_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)
