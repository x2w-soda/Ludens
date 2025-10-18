#include <Ludens/AudioBackend/MiniAudio.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Profiler/Profiler.h>
#include <atomic>
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

struct MiniAudioDeviceObj
{
    ma_device native;
};

/// @brief Miniaudio backend implementaion.
class MiniAudioObj
{
public:
    /// @brief In-place startup.
    bool startup(const MiniAudioInfo& info);

    /// @brief In-place cleanup.
    void cleanup();

    inline bool is_active() const
    {
        return mIsActive;
    }

private:
    static void ma_data_callback(ma_device* native, void* pOutput, const void* pInput, ma_uint32 frameCount);

private:
    ma_context mCtx;
    MiniAudioDeviceObj mDevice;
    MiniAudioDataCallback mDataCallback = nullptr;
    std::atomic_bool mIsActive = false;
};

static Log sLog("MiniAudio");
static MiniAudioObj sMiniAudio;

bool MiniAudioObj::startup(const MiniAudioInfo& info)
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
    deviceConfig.dataCallback = &MiniAudioObj::ma_data_callback;
    deviceConfig.pUserData = info.userData;

    mIsActive = true;

    if (ma_device_init(&mCtx, &deviceConfig, &mDevice.native) != MA_SUCCESS)
    {
        sLog.error("ma_device_init failed");
        mIsActive = false;
        return false;
    }

    // NOTE: Playback device only needs to be active when one or more sounds are playing,
    //       currently we have the playback device run until cleanup.
    if (ma_device_start(&mDevice.native) != MA_SUCCESS)
    {
        sLog.error("ma_device_start failed");
        mIsActive = false;
        return false;
    }

    sLog.info("successful startup");
    mDataCallback = info.dataCallback;

    return true;
}

void MiniAudioObj::cleanup()
{
    if (!mIsActive)
        return;

    ma_device_stop(&mDevice.native);
    ma_device_uninit(&mDevice.native);
    ma_context_uninit(&mCtx);

    sLog.info("successful cleanup");
    mIsActive = false;
}

void MiniAudioObj::ma_data_callback(ma_device* native, void* outFrames, const void* inFrames, ma_uint32 frameCount)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(sMiniAudio.mIsActive);
    LD_ASSERT(native->playback.format == ma_format_f32);
    LD_ASSERT(native->playback.channels == 2);

    if (sMiniAudio.mDataCallback)
    {
        MiniAudioDevice deviceHandle((MiniAudioDeviceObj*)native);
        sMiniAudio.mDataCallback(deviceHandle, outFrames, inFrames, (uint32_t)frameCount);
    }
    else
    {
        memset(outFrames, 0, sizeof(float) * native->playback.channels * frameCount);
    }
}

MiniAudio MiniAudio::create(const MiniAudioInfo& info)
{
    if (sMiniAudio.is_active())
        return {};

    if (!sMiniAudio.startup(info))
        return {};

    return MiniAudio(&sMiniAudio);
}

void MiniAudio::destroy(MiniAudio ma)
{
    if (ma.unwrap() != &sMiniAudio || !sMiniAudio.is_active())
        return;

    sMiniAudio.cleanup();
}

void* MiniAudioDevice::get_user_data()
{
    return mObj->native.pUserData;
}

uint32_t MiniAudioDevice::get_sample_rate()
{
    return (uint32_t)mObj->native.sampleRate;
}

} // namespace LD