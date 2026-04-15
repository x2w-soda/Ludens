#pragma once

#include <Ludens/UI/UIImmediate.h>

namespace LD {

struct EUISegmentControlStorage
{
    UIButtonData buttons[3];
};

int eui_segment_control(EUISegmentControlStorage& storage, const char* label, const char* options[3], float optionWidth = 0.0f);

} // namespace LD