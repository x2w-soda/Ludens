#include "GlyphTable.h"
#include <Ludens/Profiler/Profiler.h>
#include <algorithm>

namespace LD {

static bool msdf_glyph_geometry_order(const msdf_atlas::GlyphGeometry& lhs, const msdf_atlas::GlyphGeometry& rhs)
{
    return lhs.getCodepoint() < rhs.getCodepoint();
}

void GlyphTable::build(std::vector<msdf_atlas::GlyphGeometry>& msdfGlyphs, uint32_t width, uint32_t height)
{
    LD_PROFILE_SCOPE;

    if (msdfGlyphs.empty())
        return;

    mAtlasWidth = width;
    mAtlasHeight = height;
    mRanges.clear();
    mGlyphs.resize(msdfGlyphs.size());

    // normalize input glyph order
    std::sort(msdfGlyphs.begin(), msdfGlyphs.end(), &msdf_glyph_geometry_order);

    Range range;
    range.indexBegin = 0;
    range.codeBegin = msdfGlyphs[0].getCodepoint();
    load_glyph(msdfGlyphs[0], mGlyphs[0]);

    uint32_t glyphCount = (uint32_t)msdfGlyphs.size();

    for (uint32_t i = 1; i < glyphCount; i++)
    {
        const msdf_atlas::GlyphGeometry& prev = msdfGlyphs[i - 1];
        const msdf_atlas::GlyphGeometry& now = msdfGlyphs[i];

        if (now.getCodepoint() != prev.getCodepoint() + 1)
        {
            range.codeEnd = prev.getCodepoint();
            mRanges.push_back(range);

            range.indexBegin = i;
            range.codeBegin = now.getCodepoint();
        }

        load_glyph(now, mGlyphs[i]);
    }

    range.codeEnd = msdfGlyphs.back().getCodepoint();
    mRanges.push_back(range);
}

bool GlyphTable::find(uint32_t code, GlyphData& glyph)
{
    for (const Range& range : mRanges)
    {
        if (range.codeBegin <= code && code <= range.codeEnd)
        {
            glyph = mGlyphs[range.indexBegin + code - range.codeBegin];
            return true;
        }
    }

    return false;
}

uint32_t GlyphTable::get_range_count() const
{
    return (uint32_t)mRanges.size();
}

uint32_t GlyphTable::get_glyph_count() const
{
    return (uint32_t)mGlyphs.size();
}

void GlyphTable::load_glyph(const msdf_atlas::GlyphGeometry& geo, GlyphData& data)
{
    data.code = geo.getCodepoint();

    double l, b, r, t;
    geo.getQuadPlaneBounds(l, b, r, t);
    data.baselineL = (float)l;
    data.baselineB = (float)-b;
    data.baselineR = (float)r;
    data.baselineT = (float)-t;
    data.advanceX = (float)geo.getAdvance();

    geo.getBoxRect(data.atlasBB.x, data.atlasBB.y, data.atlasBB.w, data.atlasBB.h);

    // NOTE: msdf_atlas generates bitmap upside down.
    //       so we use Bitmap::flipy and report glyph Y position flipped
    data.atlasBB.y = mAtlasHeight - data.atlasBB.y - data.atlasBB.h;
}

} // namespace LD