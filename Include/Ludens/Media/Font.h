#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Media/Bitmap.h>

namespace LD {

struct Font : Handle<struct FontObj>
{
    static Font create_from_path(const char* path);
    static void destroy(Font font);
};

struct FontAtlas : Handle<struct FontAtlasObj>
{
    /// @brief create single channel signed distance field atlas for font
    static FontAtlas create_sdf(Font font);

    static void destroy(FontAtlas atlas);

    Bitmap get_bitmap();

    /// @brief get glyph bounding box in atlas
    /// @param code unicode codepoint 
    bool get_glyph(uint32_t code, int& x, int& y, int& w, int& h);
};

} // namespace LD