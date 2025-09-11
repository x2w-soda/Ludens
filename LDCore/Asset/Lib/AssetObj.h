#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/TextureAsset.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Media/Model.h>

namespace LD {

// NOTE: Placeholder Mesh asset implementation.
//       Need to figure out texture and material assets first.
struct MeshAssetObj : AssetObj
{
    ModelBinary* modelBinary;
};

/// @brief Texture2D asset implementation.
struct Texture2DAssetObj : AssetObj
{
    RSamplerInfo samplerHint;
    TextureCompression compression;
    Bitmap bitmap;
};

/// @brief Lua script asset implementation. Should contain enough information
///        to instantiate lua script instances.
struct LuaScriptAssetObj : AssetObj
{
    CUID duid;    /// component ID identifies script instance
    char* source; /// lua source code string
};

} // namespace LD