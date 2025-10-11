#include <Ludens/AudioBackend/MiniAudio.h>
#include <Ludens/Log/Log.h>
#include <miniaudio.h> // defintely hide this in source file.

#ifndef AUDIO_DEVICE_FORMAT
#define AUDIO_DEVICE_FORMAT ma_format_f32
#endif
#ifndef AUDIO_DEVICE_CHANNELS
#define AUDIO_DEVICE_CHANNELS 2
#endif
#ifndef AUDIO_DEVICE_SAMPLE_RATE
#define AUDIO_DEVICE_SAMPLE_RATE 0
#endif

namespace LD {

/// @brief Miniaudio backend implementaion.
class MiniAudioObj
{
public:
    /// @brief In-place startup.
    bool startup();

    /// @brief In-place cleanup.
    void cleanup();

    inline bool is_active() const
    {
        return mIsActive;
    }

private:
    ma_context mCtx;
    ma_device mDevice;
    bool mIsActive = false;
};

static Log sLog("MiniAudio");
static MiniAudioObj sMiniAudio;

bool MiniAudioObj::startup()
{
    if (mIsActive)
        return false;

    ma_context_config ctxConfig = ma_context_config_init();
    ma_result result = ma_context_init(nullptr, 0, &ctxConfig, &mCtx);
    if (result != MA_SUCCESS)
    {
        sLog.error("ma_context_init failed");
        return false;
    }

    ma_device_info* pPlaybackInfos;
    ma_uint32 playbackCount;
    ma_device_info* pCaptureInfos;
    ma_uint32 captureCount;
    if (ma_context_get_devices(&mCtx, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS)
    {
        sLog.error("ma_context_get_devices failed");
        return false;
    }

    for (ma_uint32 iDevice = 0; iDevice < playbackCount; iDevice++)
    {
        sLog.info("found audio playback device [{}]", pPlaybackInfos[iDevice].name);
    }

    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.pDeviceID = nullptr;
    deviceConfig.playback.format = AUDIO_DEVICE_FORMAT;
    deviceConfig.playback.channels = AUDIO_DEVICE_CHANNELS;
    deviceConfig.capture.pDeviceID = nullptr;
    deviceConfig.capture.format = ma_format_s16; // TODO:
    deviceConfig.capture.channels = 1;
    deviceConfig.sampleRate = AUDIO_DEVICE_SAMPLE_RATE;
    deviceConfig.dataCallback = nullptr;
    deviceConfig.pUserData = nullptr;

    if (ma_device_init(&mCtx, &deviceConfig, &mDevice) != MA_SUCCESS)
    {
        sLog.error("ma_device_init failed");
        return false;
    }

    // NOTE: Playback device only needs to be active when one or more sounds are playing,
    //       currently we have the playback device run until cleanup.
    if (ma_device_start(&mDevice) != MA_SUCCESS)
    {
        sLog.error("ma_device_start failed");
        return false;
    }

    sLog.info("successful startup");
    mIsActive = true;
    return true;
}

void MiniAudioObj::cleanup()
{
    if (!mIsActive)
        return;

    ma_device_stop(&mDevice);
    ma_device_uninit(&mDevice);
    ma_context_uninit(&mCtx);

    sLog.info("successful cleanup");
    mIsActive = false;
}

MiniAudio MiniAudio::create()
{
    if (sMiniAudio.is_active())
        return {};

    if (!sMiniAudio.startup())
        return {};

    return MiniAudio(&sMiniAudio);
}

void MiniAudio::destroy(MiniAudio ma)
{
    if (ma.unwrap() != &sMiniAudio || !sMiniAudio.is_active())
        return;

    sMiniAudio.cleanup();
}

} // namespace LD