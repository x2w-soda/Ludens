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
    Color fieldColor;
    Color selectionColor;
};

/// @brief Handle to a UI theme.
struct UITheme : Handle<UIThemeInfo>
{
    /// @brief Get default UI theme values.
    static UITheme get_default_theme();

    inline Color get_surface_color() const { return mObj->surfaceColor; };
    inline Color get_on_surface_color() const { return mObj->onSurfaceColor; };
    inline Color get_primary_color() const { return mObj->primaryColor; };
    inline Color get_background_color() const { return mObj->backgroundColor; };
    inline Color get_field_color() const { return mObj->fieldColor; }
    inline Color get_selection_color() const { return mObj->selectionColor; }
};

} // namespace LD