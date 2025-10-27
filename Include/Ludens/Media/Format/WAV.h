#pragma once

#include <Ludens/Media/AudioData.h>
#include <cstdint>

namespace LD {

struct WAVData : AudioData
{
    /// @brief Parse WAV data and create handle.
    static WAVData create(const void* data, size_t size);

    /// @brief Destroy WAV data.
    static void destroy(WAVData data);
};

} // namespace LD