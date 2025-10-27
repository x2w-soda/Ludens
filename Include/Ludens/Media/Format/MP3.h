#pragma once

#include <Ludens/DSP/DSP.h>
#include <Ludens/Media/AudioData.h>

namespace LD {

struct MP3Data : AudioData
{
    /// @brief Parse MP3 data and create handle.
    static MP3Data create(const void* data, size_t size);

    /// @brief Destroy MP3 data.
    static void destroy(MP3Data data);
};

} // namespace LD