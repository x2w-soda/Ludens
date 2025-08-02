#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Asset/TextureAsset.h>
#include <Ludens/Media/Model.h>
#include <Ludens/Media/Bitmap.h>

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

} // namespace LD