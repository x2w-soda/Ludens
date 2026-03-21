#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Media/Font.h>
#include <Ludens/RenderBackend/RBackend.h>

namespace LD {

/// @brief Bundles font meta data and VRAM image for rendering.
struct UIFont : Handle<struct UIFontObj>
{
    FontAtlas font_atlas();
    RImage image();
};

struct UIFontRegistry : Handle<struct UIFontRegistryObj>
{
    static UIFontRegistry create();
    static void destroy(UIFontRegistry registry);

    UIFont add_font(FontAtlas atlas, RImage image);
};

} // namespace LD