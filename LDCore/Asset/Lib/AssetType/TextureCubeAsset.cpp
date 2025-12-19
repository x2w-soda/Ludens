#include <Ludens/Asset/AssetType/TextureCubeAsset.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Serial.h>

#include <cstring>

#include "../AssetObj.h"
#include "TextureCubeAssetObj.h"

namespace LD {

static const char* sFaceChunkNames[6] = {
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

static void serialize_samp(Serializer& serial, const RSamplerInfo& samplerHint)
{
    serial.write_chunk_begin("SAMP");
    serial.write_u32((uint32_t)samplerHint.filter);
    serial.write_u32((uint32_t)samplerHint.mipmapFilter);
    serial.write_u32((uint32_t)samplerHint.addressMode);
    serial.write_chunk_end();
}

bool TextureCubeAssetObj::serialize(Serializer& serial, const TextureCubeAssetObj& obj)
{
    serialize_samp(serial, obj.samplerHint);
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

    uint64_t fileSize = FS::get_file_size(job.loadPath);
    if (fileSize == 0)
        return;

    obj->serialData = heap_malloc(fileSize, MEMORY_USAGE_ASSET);
    if (!FS::read_file(job.loadPath, fileSize, (byte*)obj->serialData))
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

void TextureCubeAssetImportJob::submit()
{
    mHeader.user = this;
    mHeader.type = 0;
    mHeader.fn = &TextureCubeAssetImportJob::execute;

    JobSystem::get().submit(&mHeader, JOB_DISPATCH_STANDARD);
}

void TextureCubeAssetImportJob::execute(void* user)
{
    LD_PROFILE_SCOPE;

    auto& self = *(TextureCubeAssetImportJob*)user;
    auto* obj = (TextureCubeAssetObj*)self.asset.unwrap();

    obj->auid = 0;
    obj->samplerHint = self.info.samplerHint;
    obj->serialData = nullptr;

    // serialize and load at the same time.
    Serializer serial;
    asset_header_write(serial, ASSET_TYPE_TEXTURE_CUBE);
    serialize_samp(serial, obj->samplerHint);

    size_t fileDataOffsets[6];

    for (int i = 0; i < 6; i++)
    {
        const FS::Path& path = self.info.sourcePaths[i];
        uint64_t fileSize = FS::get_file_size(path);
        obj->fileSize[i] = fileSize;

        fileDataOffsets[i] = serial.write_chunk_begin(sFaceChunkNames[i]);

        byte* fileData = serial.advance(fileSize);
        if (!FS::read_file(path, fileSize, fileData) || fileSize == 0)
            return; // TODO: fix leaks

        serial.write_chunk_end();
    }

    size_t seiralSize;
    const byte* serialData = serial.view(seiralSize);

    // only retrieve address after serializer has completed all writes
    for (int i = 0; i < 6; i++)
        obj->fileData[i] = serialData + fileDataOffsets[i];

    FS::write_file(self.info.savePath, seiralSize, serialData);

    obj->bitmap = Bitmap::create_cubemap_from_file_data(obj->fileSize, obj->fileData);
    LD_ASSERT(obj->bitmap);
}

} // namespace LD