#pragma once

#include <Ludens/Asset/AssetManager.h>
#include <Ludens/Asset/AssetType/AudioClipAsset.h>
#include <Ludens/AudioSystem/AudioSystem.h>
#include <Ludens/DSA/HashMap.h>
#include <Ludens/DataRegistry/DataComponent.h>

namespace LD {

/// @brief Cache of audio system resources. This class connects Scene, AssetManager, and AudioSystem.
class AudioSystemCache
{
public:
    /// @brief In-place creation, connect to audio system.
    void create(AudioSystem system, AssetManager manager);

    /// @brief In-place destruction, destroys all resources from audio system.
    /// @warning All playbacks should have already been destroyed, this destroys remaining audio buffers.
    void destroy();

    /// @brief Get or create corresponding audio buffer from asset.
    AudioBuffer get_or_create_audio_buffer(AssetID clipID);

    inline void update() { mSystem.update(); }
    inline AudioPlayback create_playback(AudioBuffer buffer, float pan, float volumeLinear) { return mSystem.create_playback(buffer, pan, volumeLinear); }
    inline void destroy_playback(AudioPlayback playback) { mSystem.destroy_playback(playback); }
    inline void stop_playback(AudioPlayback playback) { mSystem.stop_playback(playback); }
    inline void start_playback(AudioPlayback playback) { mSystem.start_playback(playback); }
    inline void pause_playback(AudioPlayback playback) { mSystem.pause_playback(playback); }
    inline void resume_playback(AudioPlayback playback) { mSystem.resume_playback(playback); }
    inline void set_playback_buffer(AudioPlayback playback, AudioBuffer buffer) { mSystem.set_playback_buffer(playback, buffer); }

private:
    AudioSystem mSystem{};
    AssetManager mAssetManager{};
    HashMap<AssetID, AudioBuffer> mClipToBuffer; /// map audio clip to audio buffer
};

} // namespace LD