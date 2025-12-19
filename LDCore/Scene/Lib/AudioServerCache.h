#pragma once

#include <Ludens/Asset/AssetType/AudioClipAsset.h>
#include <Ludens/AudioServer/AudioServer.h>

#include <unordered_map>

namespace LD {

/// @brief Cache of audio server resources.
class AudioServerCache
{
public:
    /// @brief In-place startup, connect to audio server.
    void startup(AudioServer server);

    /// @brief In-place cleanup, destroys all resources from audio server.
    /// @warning All playbacks should have already been destroyed, this destroys remaining audio buffers.
    void cleanup();

    /// @brief Get or create corresponding audio buffer from asset.
    AudioBuffer get_or_create_audio_buffer(AudioClipAsset clipA);

private:
    std::unordered_map<AUID, AudioBuffer> mClipToBuffer; /// map audio clip to audio buffer
    AudioServer mServer{};
};

} // namespace LD