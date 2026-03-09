#pragma once

#include <Ludens/Scene/ComponentView.h>

namespace LD {

struct AudioSourceComponent;

/// @brief Public interface for audio source components.
class AudioSourceView : public ComponentView
{
public:
    AudioSourceView() = delete;
    AudioSourceView(ComponentView comp);
    AudioSourceView(AudioSourceComponent* comp);

    bool load(AssetID clipAsset, float pan, float volumeLinear);

    void play();
    void pause();
    void resume();

    bool set_clip_asset(AssetID clipID);
    AssetID get_clip_asset();

    float get_volume_linear();
    bool set_volume_linear(float volume);
    float get_pan();
    bool set_pan(float pan);

private:
    AudioSourceComponent* mAudioSource = nullptr;
};

} // namespace LD