#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/RenderBackend/RBackend.h>

namespace LD {

/// @brief TextureCube asset implementation.
struct TextureCubeAssetObj : AssetObj
{
    Bitmap bitmap;           // single bitmap with 6 faces
    const void* serialData;  // entire LDA file loaded
    const void* fileData[6]; // source image data for each face.
    uint32_t fileSize[6];    // source image data size for each face.
    RSamplerInfo samplerHint;

    static bool serialize(Serializer& serial, const TextureCubeAssetObj& obj);
    static void serialize_sampler(Serializer& serial, const RSamplerInfo& samplerHint);
    static bool deserialize(Deserializer& serial, TextureCubeAssetObj& obj);
    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);

    static const char* sFaceChunkNames[6];
};

} // namespace LD