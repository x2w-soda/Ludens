#include <Ludens/Asset/AssetType/TextureCubeAsset.h>
#include <Ludens/Asset/AssetType/TextureCubeAssetObj.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Serial.h>

#include <cstring>

#include "../AssetMeta.h"

namespace LD {
#if 0

const char* TextureCubeAssetObj::sFaceChunkNames[6] = {
    "PX..",
    "NX..",
    "PY..",
    "NY..",
    "PZ..",
    "NZ..",
};

static void deserialize_face(Deserializer& serial, TextureCubeAssetObj& obj, const byte* fileData, size_t fileSize, int faceIndex)
{
    obj.fileSize[faceIndex] = fileSize;
    obj.fileData[faceIndex] = fileData;
    serial.advance(fileSize);
}

void TextureCubeAssetObj::serialize_sampler(Serializer& serial, const RSamplerInfo& samplerHint)
{
    serial.write_chunk_begin("SAMP");
    serial.write_u32((uint32_t)samplerHint.filter);
    serial.write_u32((uint32_t)samplerHint.mipmapFilter);
    serial.write_u32((uint32_t)samplerHint.addressMode);
    serial.write_chunk_end();
}

bool TextureCubeAssetObj::serialize(Serializer& serial, const TextureCubeAssetObj& obj)
{
    serialize_sampler(serial, obj.samplerHint);
    for (int i = 0; i < 6; i++)
    {
        serial.write_chunk_begin(sFaceChunkNames[i]);
        serial.write((const byte*)obj.fileData[i], obj.fileSize[i]);
        serial.write_chunk_end();
    }

    return true;
}

bool TextureCubeAssetObj::deserialize(Deserializer& serial, TextureCubeAssetObj& obj)
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
            continue;
        }

        for (int i = 0; i < 6; i++)
        {
            if (name == sFaceChunkNames[i])
            {
                deserialize_face(serial, obj, chunkData, chunkSize, i);
                break;
            }
        }
    }

    for (int i = 0; i < 6; i++)
    {
        if (obj.fileSize[i] == 0 || !obj.fileData[i])
            return false;
    }

    return hasSampChunk;
}

void TextureCubeAssetObj::load(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetLoadJob*)user;
    TextureCubeAssetObj* obj = (TextureCubeAssetObj*)job.assetHandle.unwrap();

    std::string err; // TODO:
    uint64_t fileSize;
    if (!FS::get_file_size(job.loadPath, fileSize, err) || fileSize == 0)
        return;

    obj->serialData = heap_malloc(fileSize, MEMORY_USAGE_ASSET);
    if (!FS::read_file(job.loadPath, MutView((char*)obj->serialData, fileSize), err))
        return;

    Deserializer serial(obj->serialData, fileSize);

    AssetType type;
    uint16_t major, minor, patch;
    if (!asset_header_read(serial, major, minor, patch, type) || type != ASSET_TYPE_TEXTURE_CUBE)
        return;

    if (!LD::deserialize<TextureCubeAssetObj>(serial, *obj))
        return;

    obj->bitmap = Bitmap::create_cubemap_from_file_data(obj->fileSize, obj->fileData);
}

void TextureCubeAssetObj::unload(AssetObj* base)
{
    TextureCubeAssetObj& self = *(TextureCubeAssetObj*)base;

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

    for (int i = 0; i < 6; i++)
    {
        self.fileData[i] = nullptr;
        self.fileSize[i] = 0;
    }
}

Bitmap TextureCubeAsset::get_bitmap() const
{
    auto* obj = (TextureCubeAssetObj*)mObj;

    return obj->bitmap;
}

#endif
} // namespace LD