#include "../AssetObj.h"
#include <Ludens/Asset/AssetType/TextureAsset.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Compress.h>
#include <Ludens/Serial/Serial.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

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
    return mObj->samplerHint;
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
    Texture2DAssetObj* obj = self.asset.unwrap();

    LD_ASSERT(self.info.compression == TEXTURE_COMPRESSION_LZ4);

    obj->auid = 0;
    obj->compression = self.info.compression;
    obj->samplerHint = self.info.samplerHint;

    std::string sourcePath = self.info.sourcePath.string();
    obj->bitmap = Bitmap::create_from_path(sourcePath.c_str(), false);

    // serialize asset to disk
    Serializer serializer;

    serializer.write_u32(obj->auid);
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

void Texture2DAssetLoadJob::submit()
{
    mHeader.user = this;
    mHeader.type = 0;
    mHeader.fn = &Texture2DAssetLoadJob::execute;

    JobSystem::get().submit(&mHeader, JOB_DISPATCH_STANDARD);
}

void Texture2DAssetLoadJob::execute(void* user)
{
    LD_PROFILE_SCOPE;

    auto& self = *(Texture2DAssetLoadJob*)user;
    Texture2DAssetObj* obj = self.asset.unwrap();

    uint64_t binarySize = FS::get_file_size(self.loadPath);
    if (binarySize == 0)
        return;

    std::vector<byte> binary(binarySize);
    FS::read_file(self.loadPath, binarySize, binary.data());

    // deserialize asset from disk
    Serializer serializer(binarySize);
    FS::read_file(self.loadPath, binarySize, serializer.data());

    uint32_t unused;
    serializer.read_u32(unused); // TODO: AUID is already known by now

    serializer.read_i32((int32_t&)obj->compression);
    serializer.read_i32((int32_t&)obj->samplerHint.filter);
    serializer.read_i32((int32_t&)obj->samplerHint.mipmapFilter);
    serializer.read_i32((int32_t&)obj->samplerHint.addressMode);

    Bitmap::deserialize(serializer, obj->bitmap);
}

} // namespace LD