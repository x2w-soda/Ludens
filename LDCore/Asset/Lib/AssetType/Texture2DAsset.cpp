#include <Ludens/Asset/AssetType/Texture2DAsset.h>
#include <Ludens/Asset/AssetType/Texture2DAssetObj.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Compress.h>
#include <Ludens/Serial/Serial.h>
#include <Ludens/System/FileSystem.h>

#include "../AssetMeta.h"

namespace LD {

bool Texture2DAssetObj::serialize(Serializer& serial, const Texture2DAssetObj& obj)
{
    serialize_sampler_info(serial, obj.samplerHint);
    serial.write_chunk_begin("FILE");
    serial.write((const byte*)obj.fileData, (size_t)obj.fileSize);
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
            obj.fileSize = chunkSize;
            obj.fileData = (void*)chunkData;
            serial.advance(chunkSize);
        }
    }

    return hasSampChunk && obj.fileSize > 0 && obj.fileData;
}

void Texture2DAssetObj::load(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetLoadJob*)user;
    Texture2DAssetObj* obj = (Texture2DAssetObj*)job.assetHandle.unwrap();

    std::string err; // TODO:
    uint64_t serialSize;
    if (!FS::get_file_size(job.loadPath, serialSize, err) || serialSize == 0)
        return;

    obj->serialData = heap_malloc(serialSize, MEMORY_USAGE_ASSET);
    if (!FS::read_file(job.loadPath, MutView((char*)obj->serialData, serialSize), err))
        return;

    Deserializer serial(obj->serialData, serialSize);

    AssetType type;
    uint16_t major, minor, patch;
    if (!asset_header_read(serial, major, minor, patch, type))
        return;

    if (type != ASSET_TYPE_TEXTURE_2D)
        return;

    if (!Texture2DAssetObj::deserialize(serial, *obj))
        return;

    obj->bitmap = Bitmap::create_from_file_data(obj->fileSize, obj->fileData);
}

void Texture2DAssetObj::unload(AssetObj* base)
{
    Texture2DAssetObj& self = *(Texture2DAssetObj*)base;

    if (self.bitmap)
    {
        Bitmap::destroy(self.bitmap);
        self.bitmap = {};
    }

    if (self.serialData)
    {
        heap_free((void*)self.serialData);
        self.serialData = nullptr;
    }

    self.fileData = nullptr;
    self.fileSize = 0;
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