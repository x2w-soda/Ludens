#include "AudioUtil.h"
#include <Ludens/DSP/Resampler.h>
#include <Ludens/Header/Types.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Media/Format/WAV.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/System/Memory.h>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace LD {

static Log sLog("LDBuilder");

struct AudioUtilObj
{
    Resampler resampler;
};

AudioUtil AudioUtil::create()
{
    AudioUtilObj* obj = (AudioUtilObj*)heap_malloc(sizeof(AudioUtilObj), MEMORY_USAGE_MISC);

    obj->resampler = {};

    return {obj};
}

void AudioUtil::destroy(AudioUtil util)
{
    AudioUtilObj* obj = util;

    heap_free(util);
}

bool AudioUtil::resample(const std::filesystem::path& srcFile, const std::filesystem::path& dstFile, uint32_t sampleRate, SampleFormat format)
{
    std::string srcExt = srcFile.extension().string();
    if (!srcExt.ends_with(".wav"))
    {
        sLog.warn("unsupported audio file type {}, currently only .wav is supported", srcExt);
        return false;
    }

    std::string dstExt = dstFile.extension().string();
    if (!dstExt.ends_with(".wav"))
    {
        sLog.warn("unsupported audio file type {}, currently only .wav is supported", dstExt);
        return false;
    }

    uint64_t byteSize;
    if (!FS::read_file(srcFile, byteSize, nullptr))
    {
        sLog.warn("failed to open file [{}]", srcFile.string());
        return false;
    }

    std::vector<byte> fileData(byteSize);
    FS::read_file(srcFile, byteSize, fileData.data());
    WAVData wav = WAVData::create(fileData.data(), fileData.size());

    if (!wav)
    {
        sLog.warn("failed to parse wav file");
        return false;
    }

    ResamplerInfo resamplerI{};
    resamplerI.channels = wav.get_channels();
    resamplerI.dstSampleRate = sampleRate;
    mObj->resampler = Resampler::create(resamplerI);

    if (!mObj->resampler)
    {
        sLog.warn("failed to create resampler");
        return false;
    }

    uint32_t channels = wav.get_channels();
    uint32_t srcSampleCount = wav.get_sample_count();
    uint32_t srcSampleRate = wav.get_sample_rate();
    uint32_t dstSampleCount = mObj->resampler.get_dst_sample_count(srcSampleCount, (float)srcSampleRate);
    uint32_t dstFrameCount = dstSampleCount / channels;
    uint32_t srcFrameCount = srcSampleCount / channels;

    byteSize = (uint64_t)sample_format_byte_size(format, dstSampleCount);
    std::vector<byte> dstSamples(byteSize);

    ResamplerProcessInfo processI{};
    processI.srcSampleRate = (float)srcSampleRate;
    processI.srcFormat = wav.get_sample_format();
    processI.srcSamples = wav.get_data(byteSize);
    processI.srcFrameCount = srcFrameCount;
    processI.dstFormat = format;
    processI.dstSamples = (void*)dstSamples.data();
    processI.dstFrameCount = dstFrameCount;
    uint32_t writtenSampleCount = mObj->resampler.process(processI);

    if (writtenSampleCount == 0)
    {
        sLog.warn("resampler failed to process");
        return false;
    }

    dstSamples.resize(sample_format_byte_size(format, writtenSampleCount));

    size_t sampleSize = sample_format_byte_size(format, 1);
    WAVHeader header;
    wav.get_header(header);
    header.bitsPerSample = sampleSize * 8;
    header.audioFormat = format == SAMPLE_FORMAT_F32 ? 3 : 1;
    header.blockAlign = sampleSize * channels;
    header.sampleRate = sampleRate;
    header.byteRate = header.blockAlign * header.sampleRate;
    bool success = WAVData::save_to_disk(dstFile, header, dstSamples.data(), dstSamples.size());

    if (!success)
        sLog.warn("failed to save [{}] to disk", dstFile.string());
    else
        sLog.info("saved to disk [{}]", dstFile.string());

    WAVData::destroy(wav);
    Resampler::destroy(mObj->resampler);
    mObj->resampler = {};

    return success;
}

} // namespace LD