#include <Ludens/Asset/AssetType/AudioClipAsset.h>
#include <Ludens/Asset/AssetType/AudioClipAssetObj.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/DSP/DSP.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>

#include "../AssetLoadJob.h"
#include "../AssetMeta.h"

namespace LD {

bool AudioClipAssetObj::load_from_binary(AssetLoadJob& job, const FS::Path& filePath)
{
    Vector<byte> tmp;
    if (!job.read_file_to_vector(filePath, tmp))
        return false;

    Deserializer serial(tmp.data(), tmp.size());

    AssetType type;
    uint16_t major, minor, patch;
    if (!asset_header_read(serial, major, minor, patch, type))
        return false;

    if (type != ASSET_TYPE_AUDIO_CLIP)
        return false;

    uint32_t u32;
    uint32_t channels;
    uint32_t sampleRate;
    uint32_t frameCount;
    serial.read_u32(u32);
    serial.read_u32(sampleRate);
    serial.read_u32(channels);
    serial.read_u32(frameCount);
    SampleFormat format = (SampleFormat)u32;
    LD_ASSERT(format == SAMPLE_FORMAT_F32);

    uint64_t sampleByteSize;
    serial.read_u64(sampleByteSize);

    const byte* sampleData = serial.view_now();
    serial.advance(sampleByteSize);

    data = AudioData::create_from_samples(channels, sampleRate, frameCount, sampleData, sampleByteSize);

    if (!job.require(data, "failed to create AudioData"))
        return false;

    return true;
}

void AudioClipAssetObj::create(AssetObj* base)
{
    new (base) AudioClipAssetObj();
}

void AudioClipAssetObj::destroy(AssetObj* base)
{
    ((AudioClipAssetObj*)base)->~AudioClipAssetObj();
}

void AudioClipAssetObj::load(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetLoadJob*)user;
    AudioClipAssetObj* obj = (AudioClipAssetObj*)job.assetHandle.unwrap();

    FS::Path filePath = job.assetDirPath / LD_ASSET_DEFAULT_BINARY_FILE_NAME;
    if (FS::exists(filePath))
        obj->load_from_binary(job, filePath);
}

void AudioClipAssetObj::unload(AssetObj* base)
{
    AudioClipAssetObj& self = *(AudioClipAssetObj*)base;

    AudioData::destroy(self.data);
    self.data = {};
}

uint32_t AudioClipAsset::get_frame_count()
{
    auto* obj = (AudioClipAssetObj*)mObj;

    return obj->data.get_frame_count();
}

uint32_t AudioClipAsset::get_channel_count()
{
    auto* obj = (AudioClipAssetObj*)mObj;

    return obj->data.get_channels();
}

uint32_t AudioClipAsset::get_sample_rate()
{
    auto* obj = (AudioClipAssetObj*)mObj;

    return obj->data.get_sample_rate();
}

const float* AudioClipAsset::get_frames(uint32_t frameOffset)
{
    auto* obj = (AudioClipAssetObj*)mObj;

    LD_ASSERT(obj->data.get_sample_format() == SAMPLE_FORMAT_F32);
    const float* samples = (const float*)obj->data.get_samples();

    return samples + frameOffset * obj->data.get_channels();
}

} // namespace LD