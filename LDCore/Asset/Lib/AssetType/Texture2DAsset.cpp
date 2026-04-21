#include <Ludens/Asset/AssetType/Texture2DAsset.h>
#include <Ludens/Asset/AssetType/Texture2DAssetObj.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Compress.h>
#include <Ludens/Serial/Serial.h>
#include <Ludens/System/FileSystem.h>

#include "../AssetLoadJob.h"
#include "../AssetMeta.h"

namespace LD {

bool Texture2DAssetObj::load_from_binary(AssetLoadJob& job, const FS::Path& filePath)
{
    uint64_t serialSize;
    if (!job.get_file_size(filePath, serialSize))
        return false;

    serialData.resize(serialSize);
    if (!job.read_file(filePath, MutView((char*)serialData.data(), serialSize)))
        return false;

    Deserializer serial(serialData.data(), serialSize);

    AssetType type;
    uint16_t major, minor, patch;
    if (!asset_header_read(serial, major, minor, patch, type))
        return false;

    if (type != ASSET_TYPE_TEXTURE_2D)
        return false;

    if (!Texture2DAssetObj::deserialize(serial, *this))
        return false;

    bitmap = Bitmap::create_from_file_data(fileView);
    if (!job.require(bitmap, "failed to create Bitmap"))
        return false;

    return true;
}

bool Texture2DAssetObj::serialize(Serializer& serial, const Texture2DAssetObj& obj)
{
    serialize_sampler_info(serial, obj.samplerHint);
    serial.write_chunk_begin("FILE");
    serial.write((const byte*)obj.fileView.data, obj.fileView.size);
    serial.write_chunk_end();

    return true;
}

void Texture2DAssetObj::serialize_sampler_info(Serializer& serial, const RSamplerInfo& sampler)
{
    serial.write_chunk_begin("SAMP");
    serial.write_u32((uint32_t)sampler.filter);
    serial.write_u32((uint32_t)sampler.mipmapFilter);
    serial.write_u32((uint32_t)sampler.addressMode);
    serial.write_chunk_end();
}

bool Texture2DAssetObj::deserialize(Deserializer& serial, Texture2DAssetObj& obj)
{
    std::string name(4, ' ');
    uint32_t chunkSize;
    const byte* chunkData;

    bool hasSampChunk = false;

    while ((chunkData = serial.read_chunk(name.data(), chunkSize)))
    {
        if (name == "SAMP")
        {
            hasSampChunk = true;

            uint32_t filterU32, mipmapFilterU32, addressModeU32;
            serial.read_u32(filterU32);
            serial.read_u32(mipmapFilterU32);
            serial.read_u32(addressModeU32);
            obj.samplerHint.filter = (RFilter)filterU32;
            obj.samplerHint.mipmapFilter = (RFilter)mipmapFilterU32;
            obj.samplerHint.addressMode = (RSamplerAddressMode)addressModeU32;
        }
        else if (name == "FILE")
        {
            obj.fileView.size = chunkSize;
            obj.fileView.data = (const char*)chunkData;
            serial.advance(chunkSize);
        }
    }

    return hasSampChunk && obj.fileView;
}

void Texture2DAssetObj::create(AssetObj* base)
{
    new (base) Texture2DAssetObj();
}

void Texture2DAssetObj::destroy(AssetObj* base)
{
    ((Texture2DAssetObj*)base)->~Texture2DAssetObj();
}

void Texture2DAssetObj::load(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetLoadJob*)user;
    Texture2DAssetObj* obj = (Texture2DAssetObj*)job.assetHandle.unwrap();

    FS::Path filePath = job.assetDirPath / LD_ASSET_DEFAULT_BINARY_FILE_NAME;
    if (FS::exists(filePath))
        obj->load_from_binary(job, filePath);
}

void Texture2DAssetObj::unload(AssetObj* base)
{
    Texture2DAssetObj& self = *(Texture2DAssetObj*)base;

    if (self.bitmap)
    {
        Bitmap::destroy(self.bitmap);
        self.bitmap = {};
    }

    self.serialData.clear();
    self.fileView = {};
}

void Texture2DAsset::unload()
{
    Texture2DAssetObj::unload(mObj);

    mObj->manager->free_asset(mObj);
    mObj = nullptr;
}

Bitmap Texture2DAsset::get_bitmap()
{
    auto* obj = (Texture2DAssetObj*)mObj;

    return obj->bitmap;
}

RSamplerInfo Texture2DAsset::get_sampler_hint() const
{
    auto* obj = (Texture2DAssetObj*)mObj;

    return obj->samplerHint;
}

} // namespace LD