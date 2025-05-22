#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/Math/Vec2.h>
#include <Ludens/Media/Bitmap.h>

namespace LD {

struct FontMetrics
{
    double ascent;
    double descent;
    double lineHeight;
};

struct Font : Handle<struct FontObj>
{
    static Font create_from_path(const char* path);
    static void destroy(Font font);

    void get_metrics(FontMetrics& metrics, float fontSizePx);
};

enum FontAtlasType
{
    FONT_ATLAS_BITMAP = 0,
    FONT_ATLAS_SDF,
};

struct FontAtlas : Handle<struct FontAtlasObj>
{
    /// @brief create rasterized bitmap atlas for given font size
    static FontAtlas create_bitmap(Font font, float fontSize);

    /// @brief create single channel signed distance field atlas for font
    static FontAtlas create_sdf(Font font, float fontSize);

    /// @brief destroy font atlas
    static void destroy(FontAtlas atlas);

    /// @brief get font atlas type
    FontAtlasType type();

    /// @brief get the source font this atlas is generated with
    Font get_font();

    /// @brief get the font size this atlas is generated with
    float get_font_size();

    /// @brief The filter ratio is the ratio of the rendered pixel size to the actual bitmap pixel size.
    ///        If this ratio is less than one, we have minification filtering. If this ratio is greater
    ///        than one, we have magnification filtering.
    /// @info unless using SDF-based bitmap, try to keep this ratio close to one.
    float get_filter_ratio(float renderSize);

    /// @brief get atlas bitmap
    Bitmap get_bitmap();

    /// @brief get glyph bounding box in atlas
    /// @param code unicode codepoint
    /// @param glyphBB glyph BB in atlas, pixel positions
    bool get_atlas_glyph(uint32_t code, IRect& glyphBB);

    /// @brief get glyph bounding box in screen space, relative to baseline position
    /// @param code unicode codepoint
    /// @param fontSize rendering size in screen space
    /// @param baseline baseline cursor position in screen space
    /// @param glyphBB glyph BB in screen space
    /// @param advanceX baseline cursor horizontal offset in screen space
    bool get_baseline_glyph(uint32_t code, float fontSize, const Vec2& baseline, Rect& glyphBB, float& advanceX);
};

} // namespace LD