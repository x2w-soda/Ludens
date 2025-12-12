#include <Ludens/Asset/AssetType/FontAsset.h>
#include <Ludens/Profiler/Profiler.h>

#include "../AssetObj.h"

namespace LD {

void FontAssetObj::load(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetLoadJob*)user;
    auto* obj = (FontAssetObj*)job.assetHandle.unwrap();

    std::vector<byte> tmp;
    if (!FS::read_file_to_vector(job.loadPath, tmp) || tmp.empty())
        return;

    Deserializer serial(tmp.data(), tmp.size());

    AssetType type;
    uint16_t major, minor, patch;
    if (!asset_header_read(serial, major, minor, patch, type))
        return;

    if (type != ASSET_TYPE_FONT)
        return;

    serial.read_f32(obj->fontSize);

    uint32_t fontDataSize;
    serial.read_u32(fontDataSize);
    const byte* fontData = serial.view_now();

    obj->font = Font::create_from_memory(fontData, (size_t)fontDataSize);
    obj->fontAtlas = FontAtlas::create_bitmap(obj->font, obj->fontSize);
}

void FontAssetObj::unload(AssetObj* base)
{
    auto* obj = (FontAssetObj*)base;
    FontAtlas::destroy(obj->fontAtlas);
    obj->fontAtlas = {};

    Font::destroy(obj->font);
    obj->font = {};
}

Font FontAsset::get_font()
{
    auto* obj = (FontAssetObj*)mObj;

    return obj->font;
}

FontAtlas FontAsset::get_font_atlas()
{
    auto* obj = (FontAssetObj*)mObj;

    return obj->fontAtlas;
}

void FontAssetImportJob::submit()
{
    mHeader.type = 0;
    mHeader.user = this;
    mHeader.fn = &FontAssetImportJob::execute;

    JobSystem js = JobSystem::get();
    js.submit(&mHeader, JOB_DISPATCH_STANDARD);
}

void FontAssetImportJob::execute(void* user)
{
    auto& self = *(FontAssetImportJob*)user;
    auto* obj = (FontAssetObj*)self.asset.unwrap();

    std::vector<byte> tmpSourceData;
    const byte* sourceData = nullptr;
    size_t sourceDataSize = 0;

    if (self.info.sourceData)
    {
        sourceData = (const byte*)self.info.sourceData;
        sourceDataSize = self.info.sourceDataSize;
    }
    else
    {
        bool ok = FS::read_file_to_vector(self.info.sourcePath, tmpSourceData);
        sourceData = tmpSourceData.data();
        sourceDataSize = tmpSourceData.size();
    }

    obj->font = Font::create_from_memory(sourceData, sourceDataSize);
    obj->fontSize = self.info.fontSize;
    obj->fontAtlas = FontAtlas::create_bitmap(obj->font, obj->fontSize);

    // save asset to disk
    Serializer serializer;
    asset_header_write(serializer, ASSET_TYPE_FONT);

    serializer.write_f32(obj->fontSize);
    serializer.write_u32(sourceDataSize);
    serializer.write(sourceData, sourceDataSize);

    size_t binarySize;
    const byte* binary = serializer.view(binarySize);
    FS::write_file(self.info.savePath, binarySize, binary);
}

} // namespace LD