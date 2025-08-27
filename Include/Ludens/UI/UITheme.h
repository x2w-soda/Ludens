#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Header/Handle.h>

namespace LD {

// TODO: This is super temporary stuff, haven't really decided
//       on default UI styling just yet. Font size and padding
//       could also be part of a Theme.
struct UIThemeInfo
{
    Color surfaceColor;
    Color onSurfaceColor;
    Color primaryColor;
    Color backgroundColor;
};

/// @brief Handle to a UI theme.
struct UITheme : Handle<UIThemeInfo>
{
    /// @brief Get default UI theme values.
    static UIThemeInfo get_default_info();

    inline Color get_surface_color() const { return mObj->surfaceColor; };
    inline Color get_on_surface_color() const { return mObj->onSurfaceColor; };
    inline Color get_primary_color() const { return mObj->primaryColor; };
    inline Color get_background_color() const { return mObj->backgroundColor; };
};

} // namespace LD