#include <Ludens/Asset/AssetType/TextureCubeAsset.h>
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
    obj.faceSize[faceIndex] = fileSize;
    obj.faceData[faceIndex] = fileData;
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
        serial.write((const byte*)obj.faceData[i], obj.faceSize[i]);
        serial.write_chunk_end();
    }

    return true;
}

bool TextureCubeAssetObj::deserialize(Deserializer& serial, TextureCubeAssetObj& obj)
{
    char name[4];
    uint32_t chunkSize;
    const byte* chunkData;

    while ((chunkData = serial.read_chunk(name, chunkSize)))
    {
        if (!strncmp("SAMP", name, 4))
        {
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
            if (!strncmp(sFaceChunkNames[i], name, 4))
            {
                deserialize_face(serial, obj, chunkData, chunkSize, i);
                break;
            }
        }
    }

    return true;
}

void TextureCubeAssetObj::load(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetLoadJob*)user;
    TextureCubeAssetObj* obj = (TextureCubeAssetObj*)job.assetHandle.unwrap();

    uint64_t fileSize = FS::get_file_size(job.loadPath);
    if (fileSize == 0)
        return;

    obj->fileData = heap_malloc(fileSize, MEMORY_USAGE_ASSET);
    if (!FS::read_file(job.loadPath, fileSize, (byte*)obj->fileData))
        return;

    Deserializer serial(obj->fileData, fileSize);

    AssetType type;
    uint16_t major, minor, patch;
    if (!asset_header_read(serial, major, minor, patch, type) || type != ASSET_TYPE_TEXTURE_CUBE)
        return;

    if (!LD::deserialize<TextureCubeAssetObj>(serial, *obj))
        return;

    for (int i = 0; i < 6; i++)
    {
        if (obj->faceSize[i] == 0)
            return;
    }

    obj->bitmap = Bitmap::create_cubemap_from_file_data(obj->faceSize, obj->faceData);
}

void TextureCubeAssetObj::unload(AssetObj* base)
{
    TextureCubeAssetObj& self = *(TextureCubeAssetObj*)base;

    if (self.bitmap)
    {
        Bitmap::destroy(self.bitmap);
        self.bitmap = {};
    }

    if (self.fileData)
    {
        heap_free((void*)self.fileData);
        self.fileData = nullptr;
    }

    for (int i = 0; i < 6; i++)
    {
        self.faceData[i] = nullptr;
        self.faceSize[i] = 0;
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
    obj->fileData = nullptr;

    // serialize and load at the same time.
    Serializer serial;
    asset_header_write(serial, ASSET_TYPE_TEXTURE_CUBE);
    serialize_samp(serial, obj->samplerHint);

    size_t faceDataOffsets[6];

    for (int i = 0; i < 6; i++)
    {
        const FS::Path& path = self.info.sourcePaths[i];
        uint64_t faceSize = FS::get_file_size(path);
        obj->faceSize[i] = faceSize;

        faceDataOffsets[i] = serial.write_chunk_begin(sFaceChunkNames[i]);

        byte* faceData = serial.advance(faceSize);
        if (!FS::read_file(path, faceSize, faceData) || faceSize == 0)
            return; // TODO: fix leaks

        serial.write_chunk_end();
    }

    size_t seiralSize;
    const byte* serialData = serial.view(seiralSize);

    // only retrieve address after serializer has completed all writes
    for (int i = 0; i < 6; i++)
        obj->faceData[i] = serialData + faceDataOffsets[i];

    FS::write_file(self.info.savePath, seiralSize, serialData);

    obj->bitmap = Bitmap::create_cubemap_from_file_data(obj->faceSize, obj->faceData);
}

} // namespace LD