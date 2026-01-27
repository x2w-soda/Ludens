#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Math/Rect.h>

#include <msdf-atlas-gen/msdf-atlas-gen.h> // hide from user

namespace LD {

struct GlyphData
{
    uint32_t code;   /// unicode codepoint
    IRect atlasBB;   /// bounding box in bitmap atlas, pixel positions
    float baselineL; /// offset applied to baseline cursor to get left border of screen space glyph BB
    float baselineB; /// offset applied to baseline cursor to get bottom border of screen space glyph BB
    float baselineR; /// offset applied to baseline cursor to get right border of screen space glyph BB
    float baselineT; /// offset applied to baseline cursor to get top border of screen space glyph BB
    float advanceX;  /// horizontal offset applied to baseline cursor after rendering this glyph
};

/// @brief data structure for glyph lookup
class GlyphTable
{
public:
    void build(std::vector<msdf_atlas::GlyphGeometry>& msdfGlyphs, uint32_t width, uint32_t height);

    bool find(uint32_t code, GlyphData& glyph);

    /// @brief get number of ranges
    uint32_t get_range_count() const;

    /// @brief get total glyph count across ranges
    uint32_t get_glyph_count() const;

private:
    void load_glyph(const msdf_atlas::GlyphGeometry& geo, GlyphData& data);

    struct Range
    {
        uint32_t indexBegin;
        uint32_t codeBegin;
        uint32_t codeEnd;
    };

    uint32_t mAtlasWidth = 0;
    uint32_t mAtlasHeight = 0;
    Vector<GlyphData> mGlyphs;
    Vector<Range> mRanges;
};

} // namespace LD