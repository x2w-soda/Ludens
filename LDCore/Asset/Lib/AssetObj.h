#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/TextureAsset.h>
#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Media/Model.h>

namespace LD {

// NOTE: Placeholder Mesh asset implementation.
//       Need to figure out texture and material assets first.
struct MeshAssetObj
{
    AUID auid;
    ModelBinary* modelBinary;
};

/// @brief Texture2D asset implementation.
struct Texture2DAssetObj
{
    AUID auid;
    RSamplerInfo samplerHint;
    TextureCompression compression;
    Bitmap bitmap;
};

/// @brief Lua script asset implementation. Should contain enough information
///        to instantiate lua script instances.
struct LuaScriptAssetObj
{
    AUID auid;    /// asset ID identifies script template
    DUID duid;    /// data component ID identifies script instance
    char* source; /// lua source code string
};

} // namespace LD