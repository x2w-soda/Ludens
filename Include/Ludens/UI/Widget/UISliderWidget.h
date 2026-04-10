#pragma once

#include <Ludens/UI/UIWidget.h>

namespace LD {

struct UISliderStorage
{
    float min = 0.0f;   /// slider minimum value
    float max = 1.0f;   /// slider maximum value
    float ratio = 1.0f; /// normalized slider ratio
};

/// @brief UI slider widget
struct UISliderWidget : UIWidget
{
    UISliderStorage* get_storage();
    void set_storage(UISliderStorage* storage);

    /// @brief get slider value
    float get_value();
};

} // namespace LD