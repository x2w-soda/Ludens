#pragma once

#include <Ludens/Header/Color.h>

namespace LD {

// TODO: This is super temporary stuff, haven't really decided
//       on default UI styling just yet. Font size and padding
//       could also be part of a Theme.
struct UITheme
{
    Color surfaceColor;
    Color onSurfaceColor;
    Color primaryColor;
    Color onPrimaryColor;
    Color primaryContainerColor;
    Color onPrimaryContainerColor;
    Color backgroundColor;
    Color onBackgroundColor;
};

void get_default_theme(UITheme& theme);

} // namespace LD