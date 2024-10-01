#include "Core/Math/Include/Hex.h"
#include "Core/UI/Include/UITheme.h"

// Default UI Theme mostly follows Google's Material Design Guidelines
// https://m3.material.io/

namespace LD
{

StringHash UITheme::sHashBackgroundColor("_background_color");
StringHash UITheme::sHashPrimaryColor("_primary_color");
StringHash UITheme::sHashSurfaceColor("_surface_color");
StringHash UITheme::sHashSecondaryColor("_secondary_color");
StringHash UITheme::sHashOnBackgroundColor("_on_background_color");
StringHash UITheme::sHashOnPrimaryColor("_on_primary_color");
StringHash UITheme::sHashOnSurfaceColor("_on_surface_color");
StringHash UITheme::sHashOnSecondaryColor("_on_secondary_color");
StringHash UITheme::sHashWindowBorder("_window_border");
StringHash UITheme::sHashWindowPadding("_window_padding");

// NOTE: this will eventually be the default editor theme,
//       and everything is still subject to change.
static struct DefaultDarkThemeInfo : UIThemeInfo
{
    DefaultDarkThemeInfo()
    {
        Vec4 background = Hex(0x121212FF);
        Vec4 primary = Hex(0x03DAC6FF);

        BackgroundColor = background;
        SurfaceColor = Vec4::Lerp(primary, background, 0.92f);
        PrimaryColor = primary;
        SecondaryColor = Hex(0xBB86FCFF);
        OnBackgroundColor = Hex(0xFFFFFFFF);
        OnSurfaceColor = Hex(0xFFFFFFFF);
        OnPrimaryColor = Hex(0x000000FF);
        OnSecondaryColor = Hex(0x000000FF);
        WindowBorder = 2.0f;
        WindowPadding = 8.0f;
    }
} sDefaultDarkThemeInfo;

UITheme::UITheme() : UITheme(sDefaultDarkThemeInfo)
{
}

UITheme::UITheme(const UIThemeInfo& info)
{
    // theme color initialization
    mProps[sHashBackgroundColor].Color = info.BackgroundColor;
    mProps[sHashSurfaceColor].Color = info.SurfaceColor;
    mProps[sHashPrimaryColor].Color = info.PrimaryColor;
    mProps[sHashSecondaryColor].Color = info.SecondaryColor;
    mProps[sHashOnBackgroundColor].Color = info.OnBackgroundColor;
    mProps[sHashOnSurfaceColor].Color = info.OnSurfaceColor;
    mProps[sHashOnPrimaryColor].Color = info.OnPrimaryColor;
    mProps[sHashOnSecondaryColor].Color = info.OnSecondaryColor;

    // theme spacing initialization
    mProps[sHashWindowBorder].Value = info.WindowBorder;
    mProps[sHashWindowPadding].Value = info.WindowPadding;
}

bool UITheme::GetProperty(StringHash name, UIProperty& prop) const
{
    auto& ite = mProps.find(name);

    if (ite == mProps.end())
        return false;

    prop = ite->second;
    return true;
}

bool UITheme::SetProperty(StringHash name, const UIProperty& prop)
{
    bool exists = mProps.find(name) != mProps.end();

    mProps[name] = prop;
    return exists;
}

} // namespace LD