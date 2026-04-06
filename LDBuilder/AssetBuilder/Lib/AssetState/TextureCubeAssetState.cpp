#include <Ludens/Asset/AssetType/TextureCubeAssetObj.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Serial.h>
#include <LudensBuilder/AssetBuilder/AssetState/TextureCubeAssetState.h>

#include "../AssetImportJob.h"

namespace LD {

void texture_cube_asset_import(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetImportJob*)user;
    auto* obj = (TextureCubeAssetObj*)job.asset.unwrap();
    const auto& info = *(TextureCubeAssetImportInfo*)job.info;

    obj->id = 0;
    obj->samplerHint = info.samplerHint;
    obj->serialData = nullptr;

    // serialize and load at the same time.
    Serializer serial;
    asset_header_write(serial, ASSET_TYPE_TEXTURE_CUBE);
    TextureCubeAssetObj::serialize_sampler(serial, obj->samplerHint);

    size_t fileDataOffsets[6];

    for (int i = 0; i < 6; i++)
    {
        const FS::Path& path = FS::absolute(info.srcPaths[i]);
        uint64_t fileSize;
        if (!FS::get_file_size(path, fileSize, job.status.str))
        {
            job.status.type = ASSET_IMPORT_ERROR_SRC_PATH;
            return;
        }

        obj->fileSize[i] = fileSize;

        fileDataOffsets[i] = serial.write_chunk_begin(TextureCubeAssetObj::sFaceChunkNames[i]);

        byte* fileData = serial.advance(fileSize);
        if (!FS::read_file(path, MutView((char*)fileData, fileSize), job.status.str))
        {
            job.status.type = ASSET_IMPORT_ERROR_SRC_PATH;
            return; // TODO: fix leaks
        }

        serial.write_chunk_end();
    }

    View serialView = serial.view();

    // only retrieve address after serializer has completed all writes
    for (int i = 0; i < 6; i++)
        obj->fileData[i] = serialView.data + fileDataOffsets[i];

    if (!FS::write_file(info.dstPath, serialView, job.status.str))
    {
        job.status.type = ASSET_IMPORT_ERROR_DST_PATH;
        return;
    }

    obj->bitmap = Bitmap::create_cubemap_from_file_data(obj->fileSize, obj->fileData);
    if (!obj->bitmap)
    {
        job.status.type = ASSET_IMPORT_ERROR;
        job.status.str = "failed to create bitmap for TextureCubeAsset";
    }
}

} // namespace LD