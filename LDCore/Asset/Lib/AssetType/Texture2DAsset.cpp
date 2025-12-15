#include <Ludens/Asset/AssetType/Texture2DAsset.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Compress.h>
#include <Ludens/Serial/Serial.h>
#include <Ludens/System/FileSystem.h>

#include "../AssetObj.h"
#include "Texture2DAssetObj.h"

namespace LD {

void Texture2DAssetObj::load(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetLoadJob*)user;
    Texture2DAssetObj* obj = (Texture2DAssetObj*)job.assetHandle.unwrap();

    std::vector<byte> tmp;
    if (!FS::read_file_to_vector(job.loadPath, tmp) || tmp.empty())
        return;

    Deserializer serial(tmp.data(), tmp.size());

    AssetType type;
    uint16_t major, minor, patch;
    if (!asset_header_read(serial, major, minor, patch, type))
        return;

    if (type != ASSET_TYPE_TEXTURE_2D)
        return;

    serial.read_i32((int32_t&)obj->compression);
    serial.read_i32((int32_t&)obj->samplerHint.filter);
    serial.read_i32((int32_t&)obj->samplerHint.mipmapFilter);
    serial.read_i32((int32_t&)obj->samplerHint.addressMode);

    Bitmap::deserialize(serial, obj->bitmap);
}

void Texture2DAssetObj::unload(AssetObj* base)
{
    Texture2DAssetObj& self = *(Texture2DAssetObj*)base;

    if (self.bitmap)
    {
        Bitmap::destroy(self.bitmap);
        self.bitmap = {};
    }
}

void Texture2DAsset::unload()
{
    Texture2DAssetObj::unload(mObj);

    mObj->manager->free_asset(mObj);
    mObj = nullptr;
}

RSamplerInfo Texture2DAsset::get_sampler_hint() const
{
    auto* obj = (Texture2DAssetObj*)mObj;

    return obj->samplerHint;
}

void Texture2DAssetImportJob::submit()
{
    mHeader.user = this;
    mHeader.type = 0;
    mHeader.fn = &Texture2DAssetImportJob::execute;

    JobSystem::get().submit(&mHeader, JOB_DISPATCH_STANDARD);
}

void Texture2DAssetImportJob::execute(void* user)
{
    LD_PROFILE_SCOPE;

    auto& self = *(Texture2DAssetImportJob*)user;
    auto* obj = (Texture2DAssetObj*)self.asset.unwrap();

    LD_ASSERT(self.info.compression == TEXTURE_COMPRESSION_LZ4);

    obj->auid = 0;
    obj->compression = self.info.compression;
    obj->samplerHint = self.info.samplerHint;

    std::string sourcePath = self.info.sourcePath.string();
    obj->bitmap = Bitmap::create_from_path(sourcePath.c_str(), false);

    // serialize asset to disk
    Serializer serializer;
    asset_header_write(serializer, ASSET_TYPE_TEXTURE_2D);

    serializer.write_i32((int)obj->compression);
    serializer.write_i32((int)obj->samplerHint.filter);
    serializer.write_i32((int)obj->samplerHint.mipmapFilter);
    serializer.write_i32((int)obj->samplerHint.addressMode);

    obj->bitmap.set_compression(BITMAP_COMPRESSION_LZ4);
    Bitmap::serialize(serializer, obj->bitmap);

    size_t binarySize;
    const byte* binary = serializer.view(binarySize);
    FS::write_file(self.info.savePath, binarySize, binary);
}

} // namespace LD