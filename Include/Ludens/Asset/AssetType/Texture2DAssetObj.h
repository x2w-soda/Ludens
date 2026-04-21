#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/Header/View.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

class Serializer;
class Deserializer;

/// @brief Texture2D asset implementation.
struct Texture2DAssetObj : AssetObj
{
    RSamplerInfo samplerHint = {};
    Bitmap bitmap = {};
    Vector<byte> serialData; // entire LDA file mapped
    View fileView = {};

    bool load_from_binary(AssetLoadJob& job, const FS::Path& filePath);

    static bool serialize(Serializer& serial, const Texture2DAssetObj& obj);
    static void serialize_sampler_info(Serializer& serial, const RSamplerInfo& sampler);
    static bool deserialize(Deserializer& serial, Texture2DAssetObj& obj);
    static void create(AssetObj* base);
    static void destroy(AssetObj* base);
    static void load(void* assetLoadJob);
    static void unload(AssetObj* base);
};

} // namespace LD