#include <Ludens/AudioBackend/AudioBackend.h>
#include <Ludens/AudioMixer/AudioBuffer.h>
#include <Ludens/AudioMixer/AudioMixer.h>
#include <Ludens/DSP/Resampler.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/System/Memory.h>

namespace LD {

struct AudioBufferObj : public AudioObject
{
    uint32_t frameCount;
};

AudioBuffer AudioBuffer::create(const AudioBufferInfo& bufferI)
{
    AudioBufferObj* obj = nullptr;

    LD_ASSERT(bufferI.channels == AUDIO_MIXER_CHANNELS);

    if (bufferI.sampleRate == AUDIO_MIXER_SAMPLE_RATE && bufferI.format == SAMPLE_FORMAT_F32)
    {
        uint32_t sampleCount = bufferI.frameCount * bufferI.channels;
        obj = (AudioBufferObj*)heap_malloc(sizeof(AudioBufferObj) + sampleCount * sizeof(float), MEMORY_USAGE_AUDIO);
        new (obj) AudioBufferObj();
        obj->frameCount = bufferI.frameCount;
        memcpy(obj + 1, bufferI.samples, sampleCount * sizeof(float));
    }
    else
    {
        ResamplerInfo resamplerI{};
        resamplerI.channels = AUDIO_MIXER_CHANNELS;
        resamplerI.dstSampleRate = AUDIO_MIXER_SAMPLE_RATE;
        Resampler resampler = Resampler::create(resamplerI);

        uint32_t srcSampleCount = bufferI.channels * bufferI.frameCount;
        uint32_t dstSampleCount = resampler.get_dst_sample_count(srcSampleCount, bufferI.sampleRate);
        obj = (AudioBufferObj*)heap_malloc(sizeof(AudioBufferObj) + dstSampleCount * sizeof(float), MEMORY_USAGE_AUDIO);
        new (obj) AudioBufferObj();

        ResamplerProcessInfo processI{};
        processI.srcSampleRate = bufferI.sampleRate;
        processI.srcFormat = bufferI.format;
        processI.srcFrameCount = bufferI.frameCount;
        processI.srcSamples = bufferI.samples;
        processI.dstFormat = SAMPLE_FORMAT_F32;
        processI.dstFrameCount = dstSampleCount / bufferI.channels;
        processI.dstSamples = obj + 1;
        uint32_t sampleCount = resampler.process(processI);
        obj->frameCount = sampleCount / bufferI.channels;

        Resampler::destroy(resampler);
    }

    return AudioBuffer((AudioObject*)obj);
}

void AudioBuffer::destroy(AudioBuffer buffer)
{
    LD_ASSERT(!buffer.is_acquired());
    AudioBufferObj* obj = (AudioBufferObj*)buffer.unwrap();

    obj->~AudioBufferObj();
    heap_free(obj);
}

uint32_t AudioBuffer::frame_count()
{
    auto* obj = (AudioBufferObj*)mObj;

    return obj->frameCount;
}

const float* AudioBuffer::view_frame(uint32_t frameOffset)
{
    auto* obj = (AudioBufferObj*)mObj;
    LD_ASSERT(frameOffset < obj->frameCount);

    const float* samples = (const float*)(obj + 1);
    return samples + frameOffset * 2;
}

} // namespace LD