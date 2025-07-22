#pragma once

#include <Ludens/Header/Color.h>

namespace LD {
namespace UIF {

// TODO: This is super temporary stuff, haven't really decided
//       on default UI styling just yet. Font size and padding
//       could also be part of a Theme.
struct Theme
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

void get_default_theme(Theme& theme);

} // namespace UIF
} // namespace LD