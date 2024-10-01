#pragma once

#include <unordered_map>
#include "Core/Math/Include/Vec4.h"
#include "Core/DSA/Include/String.h"

namespace LD
{

union UIProperty
{
    UIProperty() {};

    Vec4 Color;
    float Value;
};

#define UI_THEME_VALUE(NAME)                                                                                           \
    inline void Get##NAME(float& value) const                                                                          \
    {                                                                                                                  \
        value = mProps.find(sHash##NAME)->second.Value;                                                                \
    }                                                                                                                  \
    inline void Set##NAME(float value)                                                                                 \
    {                                                                                                                  \
        mProps[sHash##NAME].Value = value;                                                                             \
    }

#define UI_THEME_COLOR(NAME)                                                                                           \
    inline void Get##NAME(Vec4& color) const                                                                           \
    {                                                                                                                  \
        color = mProps.find(sHash##NAME)->second.Color;                                                                \
    }                                                                                                                  \
    inline void Set##NAME(const Vec4& color)                                                                           \
    {                                                                                                                  \
        mProps[sHash##NAME].Color = color;                                                                             \
    }

struct UIThemeInfo
{
    Vec4 BackgroundColor, OnBackgroundColor;
    Vec4 SurfaceColor, OnSurfaceColor;
    Vec4 PrimaryColor, OnPrimaryColor;
    Vec4 SecondaryColor, OnSecondaryColor;
    float WindowBorder;
    float WindowPadding;
};

/// a registry of UI properties, allows user to define arbitrary properties for specific use.
class UITheme
{
public:
    UITheme();
    UITheme(const UIThemeInfo& info);

    /// @param get user defined property
    /// @return true if the property was found
    bool GetProperty(StringHash name, UIProperty& prop) const;

    /// @brief set user defined property
    /// @return true if the property exists and was overriden
    bool SetProperty(StringHash name, const UIProperty& prop);

    UI_THEME_COLOR(BackgroundColor)
    UI_THEME_COLOR(SurfaceColor)
    UI_THEME_COLOR(PrimaryColor)
    UI_THEME_COLOR(SecondaryColor)
    UI_THEME_COLOR(OnBackgroundColor)
    UI_THEME_COLOR(OnSurfaceColor)
    UI_THEME_COLOR(OnPrimaryColor)
    UI_THEME_COLOR(OnSecondaryColor)
    UI_THEME_VALUE(WindowBorder)
    UI_THEME_VALUE(WindowPadding)

private:
    static StringHash sHashBackgroundColor, sHashOnBackgroundColor;
    static StringHash sHashSurfaceColor, sHashOnSurfaceColor;
    static StringHash sHashPrimaryColor, sHashOnPrimaryColor;
    static StringHash sHashSecondaryColor, sHashOnSecondaryColor;
    static StringHash sHashWindowBorder;
    static StringHash sHashWindowPadding;

    // TODO: explore other hash map solutions
    std::unordered_map<StringHash, UIProperty> mProps;
};

#undef UI_THEME_COLOR
#undef UI_THEME_VALUE

} // namespace LD