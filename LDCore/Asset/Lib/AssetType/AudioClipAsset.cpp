#include <Ludens/Asset/AssetType/AudioClipAsset.h>
#include <Ludens/DSP/DSP.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Media/AudioData.h>
#include <Ludens/Profiler/Profiler.h>

#include "../AssetObj.h"
#include "AudioClipAssetObj.h"

namespace LD {

void AudioClipAssetObj::load(void* user)
{
    LD_PROFILE_SCOPE;

    auto& job = *(AssetLoadJob*)user;
    AudioClipAssetObj* obj = (AudioClipAssetObj*)job.assetHandle.unwrap();

    std::vector<byte> tmp;
    if (!FS::read_file_to_vector(job.loadPath, tmp) || tmp.empty())
        return;

    Deserializer serial(tmp.data(), tmp.size());

    AssetType type;
    uint16_t major, minor, patch;
    if (!asset_header_read(serial, major, minor, patch, type))
        return;

    if (type != ASSET_TYPE_AUDIO_CLIP)
        return;

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

    obj->data = AudioData::create_from_samples(channels, sampleRate, frameCount, sampleData, sampleByteSize);
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

void AudioClipAssetImportJob::submit()
{
    mHeader.user = this;
    mHeader.type = 0;
    mHeader.fn = &AudioClipAssetImportJob::execute;

    JobSystem::get().submit(&mHeader, JOB_DISPATCH_STANDARD);
}

void AudioClipAssetImportJob::execute(void* user)
{
    LD_PROFILE_SCOPE;

    auto& self = *(AudioClipAssetImportJob*)user;
    AudioClipAssetObj* obj = (AudioClipAssetObj*)self.asset.unwrap();

    std::string sourcePath = self.sourcePath.string();
    AudioData data = obj->data = AudioData::create_from_path(sourcePath);
    LD_ASSERT(data);

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

    std::string err;
    bool ok = FS::write_file(self.savePath, serializer.view(), err);
    LD_ASSERT(ok); // TODO:
}

} // namespace LD