set(MODULE_NAME LDAssetBuilder)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetBuilder.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetBuilderDef.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetImporter.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetState.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetState/AssetStates.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetState/BlobAssetState.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetState/FontAssetState.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetState/MeshAssetState.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetState/UITemplateAssetState.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetState/AudioClipAssetState.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetState/Texture2DAssetState.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetState/TextureCubeAssetState.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetState/LuaScriptAssetState.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetSource/AssetSources.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetSource/TextureAssetSource.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetSource/AudioAssetSource.h
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/AssetBuilder/AssetSource/FontAssetSource.h
)

set(MODULE_LIB
    Lib/AssetBuilderLibDef.h
    Lib/AssetBuilder.cpp
    Lib/AssetImporter.cpp
    Lib/AssetImportJob.h
    Lib/AssetImportJob.cpp
    Lib/AssetBuilderMeta.h
    Lib/AssetBuilderMeta.cpp
    Lib/AssetState/BlobAssetState.cpp
    Lib/AssetState/FontAssetState.cpp
    Lib/AssetState/MeshAssetState.cpp
    Lib/AssetState/UITemplateAssetState.cpp
    Lib/AssetState/AudioClipAssetState.cpp
    Lib/AssetState/Texture2DAssetState.cpp
    Lib/AssetState/TextureCubeAssetState.cpp
    Lib/AssetState/LuaScriptAssetState.cpp
    Lib/AssetSource/TextureAssetSource.cpp
    Lib/AssetSource/AudioAssetSource.cpp
    Lib/AssetSource/FontAssetSource.cpp
)

add_ludens_builder_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)
