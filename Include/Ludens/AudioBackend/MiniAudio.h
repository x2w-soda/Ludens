#pragma once

#include <Ludens/Header/Handle.h>

namespace LD {

/// @brief MiniAudio context.
struct MiniAudio : Handle<struct MiniAudioObj>
{
    /// @brief Create miniaudio context.
    static MiniAudio create();

    /// @brief Destroy miniaudio context
    static void destroy(MiniAudio ma);
};

} // namespace LD