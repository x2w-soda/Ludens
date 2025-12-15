#pragma once

namespace LD {

/// @brief TextureCube asset implementation.
struct TextureCubeAssetObj : AssetObj
{
    Bitmap bitmap;           // single bitmap with 6 faces
    const void* fileData;    // entire LDA file loaded
    const void* faceData[6]; // source image data for each face.
    uint32_t faceSize[6];    // source image data size for each face.
    RSamplerInfo samplerHint;

    static bool serialize(Serializer& serial, const TextureCubeAssetObj& obj);
    static bool deserialize(Deserializer& serial, TextureCubeAssetObj& obj);
    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

} // namespace LD