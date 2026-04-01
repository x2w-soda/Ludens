#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/RenderBackend/RBackend.h>

namespace LD {

class Serializer;
class Deserializer;

/// @brief Texture2D asset implementation.
struct Texture2DAssetObj : AssetObj
{
    RSamplerInfo samplerHint;
    Bitmap bitmap;
    void* serialData; // entire LDA file mapped
    const void* fileData;
    uint32_t fileSize;

    static bool serialize(Serializer& serial, const Texture2DAssetObj& obj);
    static void serialize_sampler_info(Serializer& serial, const RSamplerInfo& sampler);
    static bool deserialize(Deserializer& serial, Texture2DAssetObj& obj);
    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

} // namespace LD