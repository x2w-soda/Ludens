#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/System/FileSystem.h>
#include <cstdint>

namespace LD {

/// @brief Audio clip asset handle. This is typically a static buffer of
///        audio samples after decoding and resampling from WAV, MP3, etc.
struct AudioClipAsset : AssetHandle<struct AudioClipAssetObj>
{
    /// @brief Get number of frames in this clip.
    uint32_t get_frame_count();

    /// @brief Get number of channels in this clip.
    uint32_t get_channel_count();

    /// @brief Get default sample rate of this clip.
    uint32_t get_sample_rate();

    /// @brief Read frames from offset.
    const float* get_frames(uint32_t frameOffset);
};

class AudioClipAssetImportJob
{
public:
    AudioClipAsset asset; /// subject handle
    FS::Path sourcePath;  /// path to load the source audio file.
    FS::Path savePath;    /// path to save the imported asset

    /// @brief Submit to job system. Address of this job instance must not
    ///        change until the worker thread completes execution.
    void submit();

private:
    static void execute(void*);

    JobHeader mHeader;
};

} // namespace LD