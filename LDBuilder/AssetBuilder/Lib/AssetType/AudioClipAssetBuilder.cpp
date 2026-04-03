#include <Ludens/Asset/AssetType/AudioClipAssetObj.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Serial/Serial.h>
#include <LudensBuilder/AssetBuilder/AssetImportInfoStorage.h>
#include <LudensBuilder/AssetBuilder/AssetType/AudioClipAssetBuilder.h>

#include "../AssetImportJob.h"

namespace LD {

void audio_clip_asset_import(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetImportJob*)user;
    auto* obj = (AudioClipAssetObj*)job.asset.unwrap();
    const auto& info = *(AudioClipAssetImportInfo*)job.info;

    std::string sourcePath = info.srcPath.string();
    AudioData data = obj->data = AudioData::create_from_path(sourcePath);
    if (!job.require(data, "failed to create AudioData"))
        return;

    uint32_t sampleCount = data.get_channels() * data.get_frame_count();
    uint64_t sampleByteSize = (uint64_t)sample_format_byte_size(data.get_sample_format(), sampleCount);

    Serializer serializer;
    asset_header_write(serializer, ASSET_TYPE_AUDIO_CLIP);

    serializer.write_u32((uint32_t)data.get_sample_format());
    serializer.write_u32((uint32_t)data.get_sample_rate());
    serializer.write_u32((uint32_t)data.get_channels());
    serializer.write_u32((uint32_t)data.get_frame_count());

    serializer.write_u64(sampleByteSize);
    serializer.write((const byte*)data.get_samples(), sampleByteSize);

    job.write_to_dst_path(serializer.view());
}

} // namespace LD
