#pragma once

#include <Ludens/AudioBackend/AudioBackend.h>
#include <Ludens/DSP/DSP.h>
#include <cstdint>

namespace LD {

struct AudioBufferInfo
{
    SampleFormat format;
    uint32_t channels;
    uint32_t frameCount;
    uint32_t sampleRate;
    const void* samples;
};

struct AudioBuffer : AudioHandle
{
    /// @brief Create audio buffer.
    static AudioBuffer create(const AudioBufferInfo& bufferI);

    /// @brief Destroy audio buffer.
    static void destroy(AudioBuffer buffer);

    /// @brief Get number of frames in buffer.
    uint32_t frame_count();

    /// @brief View frames in buffer.
    const float* view_frame(uint32_t frameOffset);
};

} // namespace LD