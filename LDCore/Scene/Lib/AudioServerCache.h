#pragma once

#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetType/AudioClipAsset.h>
#include <Ludens/AudioServer/AudioServer.h>
#include <Ludens/DSA/HashMap.h>
#include <Ludens/DataRegistry/DataComponent.h>

namespace LD {

/// @brief Cache of audio server resources. This class connects Scene, AssetManager, and AudioServer.
class AudioServerCache
{
public:
    /// @brief In-place startup, connect to audio server.
    void startup(AudioServer server, AssetManager manager);

    /// @brief In-place cleanup, destroys all resources from audio server.
    /// @warning All playbacks should have already been destroyed, this destroys remaining audio buffers.
    void cleanup();

    /// @brief Get or create corresponding audio buffer from asset.
    AudioBuffer get_or_create_audio_buffer(AUID clipAUID);

    inline void update() { mServer.update(); }
    inline AudioPlayback create_playback(AudioBuffer buffer, AudioSourceComponent* comp) { return mServer.create_playback(buffer, comp->pan, comp->volumeLinear); }
    inline void destroy_playback(AudioPlayback playback) { mServer.destroy_playback(playback); }
    inline void stop_playback(AudioPlayback playback) { mServer.stop_playback(playback); }
    inline void start_playback(AudioPlayback playback) { mServer.start_playback(playback); }
    inline void pause_playback(AudioPlayback playback) { mServer.pause_playback(playback); }
    inline void resume_playback(AudioPlayback playback) { mServer.resume_playback(playback); }
    inline void set_playback_buffer(AudioPlayback playback, AudioBuffer buffer) { mServer.set_playback_buffer(playback, buffer); }

private:
    AudioServer mServer{};
    AssetManager mAssetManager{};
    HashMap<AUID, AudioBuffer> mClipToBuffer; /// map audio clip to audio buffer
};

} // namespace LD