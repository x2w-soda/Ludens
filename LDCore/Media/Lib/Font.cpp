#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/Media/Font.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>

#include <algorithm>
#include <msdf-atlas-gen/msdf-atlas-gen.h> // hide from user

#include "GlyphTable.h"

#define MAX_CORNER_ANGLE 3.0
#define GENERATOR_THREAD_COUNT 4

namespace LD {

static msdfgen::FreetypeHandle* msdfFreeType;

struct FontObj
{
    msdfgen::FontHandle* msdfHandle;
};

struct FontAtlasObj
{
    FontAtlasType type;
    Font font;
    Bitmap atlas;
    GlyphTable table;
    float fontSize = 0;
};

struct FontAtlasConfig
{
    float fontSize;
    float spacing;
    float pixelRange;
};

static Bitmap generate_bitmap_atlas(FontObj* obj, std::vector<msdf_atlas::GlyphGeometry>& glyphs, const FontAtlasConfig& config);
static Bitmap generate_sdf_atlas(FontObj* obj, std::vector<msdf_atlas::GlyphGeometry>& glyphs, const FontAtlasConfig& config);

static Bitmap generate_bitmap_atlas(FontObj* obj, std::vector<msdf_atlas::GlyphGeometry>& glyphs, const FontAtlasConfig& config)
{
    LD_PROFILE_SCOPE;

    msdfgen::FontHandle* font = obj->msdfHandle;

    msdf_atlas::FontGeometry fontGeometry(&glyphs);
    fontGeometry.loadCharset(font, 1.0f, msdf_atlas::Charset::ASCII);

    for (msdf_atlas::GlyphGeometry& glyph : glyphs)
        glyph.edgeColoring(&msdfgen::edgeColoringSimple, MAX_CORNER_ANGLE, 0);

    msdf_atlas::GridAtlasPacker packer;
    packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
    packer.setMinimumScale(config.fontSize);
    packer.setPixelRange(config.pixelRange);
    packer.setMiterLimit(1.0);
    packer.setSpacing((int)config.spacing);
    packer.pack(glyphs.data(), (int)glyphs.size());

    int atlasWidth, atlasHeight;
    packer.getDimensions(atlasWidth, atlasHeight);
    msdf_atlas::ImmediateAtlasGenerator<
        float, 1,
        msdf_atlas::sdfGenerator, // msdf_atlas::scanlineGenerator does not perform AA
        msdf_atlas::BitmapAtlasStorage<msdfgen::byte, 1>>
        generator(atlasWidth, atlasHeight);

    msdf_atlas::GeneratorAttributes attr{};
    attr.scanlinePass = true;
    attr.config.overlapSupport = true;

    generator.setAttributes(attr);
    generator.setThreadCount(GENERATOR_THREAD_COUNT);
    generator.generate(glyphs.data(), (int)glyphs.size());

    const msdfgen::BitmapConstRef<msdfgen::byte, 1>& bitmap = generator.atlasStorage();
    Bitmap handle = Bitmap::create_from_data(bitmap.width, bitmap.height, BITMAP_FORMAT_R8U, bitmap.pixels);
    handle.flipy();

    return handle;
}

static Bitmap generate_sdf_atlas(FontObj* obj, std::vector<msdf_atlas::GlyphGeometry>& glyphs, const FontAtlasConfig& config)
{
    LD_PROFILE_SCOPE;

    msdfgen::FontHandle* font = obj->msdfHandle;

    msdf_atlas::FontGeometry fontGeometry(&glyphs);
    fontGeometry.loadCharset(font, 1.0f, msdf_atlas::Charset::ASCII);

    for (msdf_atlas::GlyphGeometry& glyph : glyphs)
        glyph.edgeColoring(&msdfgen::edgeColoringSimple, MAX_CORNER_ANGLE, 0);

    msdf_atlas::GridAtlasPacker packer;
    packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
    packer.setMinimumScale(config.fontSize);
    packer.setPixelRange(config.pixelRange);
    packer.setMiterLimit(1.0);
    packer.setSpacing((int)config.spacing);
    packer.pack(glyphs.data(), (int)glyphs.size());

    int atlasWidth, atlasHeight;
    packer.getDimensions(atlasWidth, atlasHeight);
    msdf_atlas::ImmediateAtlasGenerator<
        float, 1,
        msdf_atlas::sdfGenerator,
        msdf_atlas::BitmapAtlasStorage<msdfgen::byte, 1>>
        generator(atlasWidth, atlasHeight);

    msdf_atlas::GeneratorAttributes attr{};
    attr.scanlinePass = true;
    attr.config.overlapSupport = true;

    generator.setAttributes(attr);
    generator.setThreadCount(GENERATOR_THREAD_COUNT);
    generator.generate(glyphs.data(), (int)glyphs.size());

    const msdfgen::BitmapConstRef<msdfgen::byte, 1>& bitmap = generator.atlasStorage();
    Bitmap handle = Bitmap::create_from_data(bitmap.width, bitmap.height, BITMAP_FORMAT_R8U, bitmap.pixels);
    handle.flipy();

    return handle;
}

static void measure_line_limits(FontAtlas atlas, FontMetrics metrics, View text, float fontSizePx, float& outMinWidth, float& outMaxWidth)
{
    outMinWidth = 0.0f;
    outMaxWidth = 0.0f;

    if (!text)
        return;

    Vec2 baseline(0.0f);
    float lineW = 0.0f;

    for (size_t i = 0; i < text.size; i++)
    {
        uint32_t c = (uint32_t)text.data[i];

        if (c == '\n')
        {
            lineW = 0.0f;
            continue;
        }

        float advanceX;
        Rect rect;
        Vec2 baseline(lineW, (float)metrics.ascent);
        atlas.get_baseline_glyph(c, fontSizePx, baseline, rect, advanceX);

        lineW += advanceX;
        outMaxWidth = std::max<float>(outMaxWidth, lineW);
        outMinWidth = std::max<float>(outMinWidth, rect.w);
    }
}

Font Font::create_from_path(const char* path)
{
    LD_PROFILE_SCOPE;

    FontObj* obj = heap_new<FontObj>(MEMORY_USAGE_MEDIA);

    if (!msdfFreeType)
    {
        msdfFreeType = msdfgen::initializeFreetype(); // TODO: deinitialize
        LD_ASSERT(msdfFreeType);
    }

    obj->msdfHandle = msdfgen::loadFont(msdfFreeType, path);
    LD_ASSERT(obj->msdfHandle);

    return {obj};
}

Font Font::create_from_memory(const void* memory, size_t size)
{
    LD_PROFILE_SCOPE;

    FontObj* obj = heap_new<FontObj>(MEMORY_USAGE_MEDIA);

    if (!msdfFreeType)
    {
        msdfFreeType = msdfgen::initializeFreetype(); // TODO: deinitialize
        LD_ASSERT(msdfFreeType);
    }

    obj->msdfHandle = msdfgen::loadFontData(msdfFreeType, (const msdfgen::byte*)memory, (int)size);
    LD_ASSERT(obj->msdfHandle);

    return {obj};
}

void Font::get_metrics(FontMetrics& metrics, float fontSizePx)
{
    msdfgen::FontMetrics msdfMetrics;
    msdfgen::getFontMetrics(msdfMetrics, mObj->msdfHandle, msdfgen::FONT_SCALING_EM_NORMALIZED);

    metrics.ascent = msdfMetrics.ascenderY * fontSizePx;
    metrics.descent = msdfMetrics.descenderY * fontSizePx;
    metrics.lineHeight = msdfMetrics.lineHeight * fontSizePx;
}

void Font::destroy(Font font)
{
    LD_PROFILE_SCOPE;

    FontObj* obj = (FontObj*)font;

    msdfgen::destroyFont(obj->msdfHandle);

    heap_delete<FontObj>(obj);
}

FontAtlas FontAtlas::create_bitmap(Font font, float fontSize)
{
    LD_PROFILE_SCOPE;

    FontAtlasObj* obj = heap_new<FontAtlasObj>(MEMORY_USAGE_MEDIA);
    obj->type = FONT_ATLAS_BITMAP;
    obj->font = font;
    obj->fontSize = fontSize;

    std::vector<msdf_atlas::GlyphGeometry> glyphs;

    FontAtlasConfig config;
    config.fontSize = fontSize;
    config.pixelRange = 1.0f;
    config.spacing = 0.0f;
    obj->atlas = generate_bitmap_atlas(font, glyphs, config);
    obj->table.build(glyphs, obj->atlas.width(), obj->atlas.height());

    return {obj};
}

FontAtlas FontAtlas::create_sdf(Font font, float fontSize)
{
    LD_PROFILE_SCOPE;

    FontAtlasObj* obj = heap_new<FontAtlasObj>(MEMORY_USAGE_MEDIA);
    obj->type = FONT_ATLAS_SDF;
    obj->font = font;
    obj->fontSize = fontSize;

    std::vector<msdf_atlas::GlyphGeometry> glyphs;

    FontAtlasConfig config;
    config.fontSize = fontSize;
    config.pixelRange = 2.0f;
    config.spacing = 0.0f;
    obj->atlas = generate_sdf_atlas(font, glyphs, config);
    obj->table.build(glyphs, obj->atlas.width(), obj->atlas.height());

    return {obj};
}

void FontAtlas::destroy(FontAtlas atlas)
{
    LD_PROFILE_SCOPE;

    FontAtlasObj* obj = (FontAtlasObj*)atlas;

    if (obj->atlas)
        Bitmap::destroy(obj->atlas);

    heap_delete<FontAtlasObj>(obj);
}

FontAtlasType FontAtlas::type()
{
    return mObj->type;
}

Font FontAtlas::get_font()
{
    return mObj->font;
}

float FontAtlas::get_font_size()
{
    return mObj->fontSize;
}

float FontAtlas::get_filter_ratio(float renderSize)
{
    return renderSize / mObj->fontSize;
}

Bitmap FontAtlas::get_bitmap()
{
    return mObj->atlas;
}

bool FontAtlas::get_atlas_glyph(uint32_t code, IRect& glyphBB)
{
    GlyphData glyph;

    if (!mObj->table.find(code, glyph))
        return false;

    glyphBB = glyph.atlasBB;
    return true;
}

bool FontAtlas::get_baseline_glyph(uint32_t code, float fontSize, const Vec2& baseline, Rect& glyphBB, float& advanceX)
{
    GlyphData glyph;

    if (!mObj->table.find(code, glyph))
        return false;

    float minx = baseline.x + glyph.baselineL * fontSize;
    float maxx = baseline.x + glyph.baselineR * fontSize;
    float miny = baseline.y + glyph.baselineT * fontSize;
    float maxy = baseline.y + glyph.baselineB * fontSize;
    glyphBB = Rect(minx, miny, maxx - minx, maxy - miny);

    advanceX = glyph.advanceX * fontSize;

    return true;
}

struct FontGlyphProbe
{
    int charIndex = 0;
    float minDistSquared = 0.0f;
    Vec2 pickPos; // in baseline local space
    int posQueryCount = 0;
    int posQueryAnswered = 0;
    int* posQueryCharIndices = nullptr;
    Vec2* posQueryBaselinePos = nullptr;
};

int FontAtlas::measure_text_index(View text, float fontSizePx, float limitWidth, Vec2 pos)
{
    if (!text)
        return -1;

    FontMetrics metrics;
    get_font().get_metrics(metrics, fontSizePx);

    Range range(0, text.size);
    FontAtlas handle(mObj);
    FontGlyphIteration it{};
    it.text = text;
    it.fontSizePx = fontSizePx;
    it.limitWidth = limitWidth;
    it.lineHeight = metrics.lineHeight;
    it.spanCount = 1;
    it.spanAtlas = &handle;
    it.spanRange = &range;
    it.glyphCB = [](Rect rect, Vec2, size_t charIndex, size_t, void* user) {
        FontGlyphProbe* probe = (FontGlyphProbe*)user;
        if (rect.contains(probe->pickPos))
        {
            probe->charIndex = (int)charIndex;
            return true;
        }
        return false;
    };

    FontGlyphProbe probe{};
    probe.charIndex = -1;
    probe.pickPos = pos;
    probe.pickPos.y -= metrics.ascent;
    font_glyph_iterator(&it, &probe);

    return probe.charIndex;
}

int FontAtlas::measure_cursor_index(View text, float fontSizePx, float limitWidth, Vec2 pos)
{
    if (!text)
        return -1;

    FontMetrics metrics;
    get_font().get_metrics(metrics, fontSizePx);

    Range range(0, text.size);
    FontAtlas handle(mObj);
    FontGlyphIteration it{};
    it.text = text;
    it.fontSizePx = fontSizePx;
    it.limitWidth = limitWidth;
    it.lineHeight = metrics.lineHeight;
    it.spanCount = 1;
    it.spanAtlas = &handle;
    it.spanRange = &range;
    it.glyphCB = [](Rect rect, Vec2 baseline, size_t charIndex, size_t spanIndex, void* user) {
        FontGlyphProbe* probe = (FontGlyphProbe*)user;
        Vec2 delta = baseline - probe->pickPos;
        float distSquared = Vec2::dot(delta, delta);
        if (probe->minDistSquared < 0.0f || distSquared < probe->minDistSquared)
        {
            probe->minDistSquared = distSquared;
            probe->charIndex = (int)charIndex;
        }
        return false;
    };

    FontGlyphProbe probe{};
    probe.charIndex = -1;
    probe.minDistSquared = -1.0f;
    probe.pickPos = pos;
    probe.pickPos.y -= metrics.ascent;
    Vec2 baseline = font_glyph_iterator(&it, &probe);

    Vec2 delta = baseline - probe.pickPos;
    float distSquared = Vec2::dot(delta, delta);
    if (probe.minDistSquared < 0.0f || distSquared < probe.minDistSquared)
        probe.charIndex = -1;

    return probe.charIndex;
}

void FontAtlas::measure_baseline_positions(View text, float fontSizePx, float limitWidth, int queryCount, int* inQueryIndices, Vec2* outQueryBaselinePos)
{
    FontMetrics metrics;
    get_font().get_metrics(metrics, fontSizePx);

    if (!text || queryCount == 0)
        return;

    Range range(0, text.size);
    FontAtlas handle(mObj);
    FontGlyphIteration it{};
    it.text = text;
    it.fontSizePx = fontSizePx;
    it.limitWidth = limitWidth;
    it.lineHeight = metrics.lineHeight;
    it.spanCount = 1;
    it.spanAtlas = &handle;
    it.spanRange = &range;
    it.glyphCB = [](Rect rect, Vec2 baseline, size_t charIndex, size_t spanIndex, void* user) -> bool {
        FontGlyphProbe* probe = (FontGlyphProbe*)user;
        
        if (charIndex == (size_t)probe->posQueryCharIndices[probe->posQueryAnswered])
            probe->posQueryBaselinePos[probe->posQueryAnswered++] = baseline;
        
        return probe->posQueryAnswered == probe->posQueryCount;
    };

    FontGlyphProbe probe{};
    probe.posQueryCount = queryCount;
    probe.posQueryCharIndices = inQueryIndices;
    probe.posQueryBaselinePos = outQueryBaselinePos;
    probe.posQueryAnswered = 0;
    Vec2 lastPos = font_glyph_iterator(&it, &probe);

    // handle end of text position query.
    if (probe.posQueryAnswered == probe.posQueryCount - 1 && probe.posQueryCharIndices[probe.posQueryAnswered] == text.size)
        probe.posQueryBaselinePos[probe.posQueryAnswered++] = lastPos;

    // If this fires, the query index sequence is *not* strictly increasing.
    LD_ASSERT(probe.posQueryAnswered == probe.posQueryCount);
}

float FontAtlas::measure_wrap_size(View text, float fontSizePx, float limitWidth)
{
    FontMetrics metrics;
    get_font().get_metrics(metrics, fontSizePx);

    if (!text)
        return (float)metrics.lineHeight;

    Range range(0, text.size);
    FontAtlas handle(mObj);
    FontGlyphIteration it{};
    it.text = text;
    it.fontSizePx = fontSizePx;
    it.limitWidth = limitWidth;
    it.lineHeight = metrics.lineHeight;
    it.spanCount = 1;
    it.spanAtlas = &handle;
    it.spanRange = &range;

    Vec2 baseline(0.0f, metrics.ascent);
    baseline += font_glyph_iterator(&it, nullptr);

    return baseline.y - (float)metrics.descent;
}

void FontAtlas::measure_wrap_limit(View text, float fontSizePx, float& outMinWidth, float& outMaxWidth)
{
    FontMetrics metrics;
    get_font().get_metrics(metrics, fontSizePx);

    measure_line_limits(*this, metrics, text, fontSizePx, outMinWidth, outMaxWidth);
}

Vec2 font_glyph_iterator(FontGlyphIteration* it, void* user)
{
    Vec2 baseline(it->startX, 0.0f);
    size_t charI = 0;

    for (size_t spanI = 0; spanI < it->spanCount; spanI++)
    {
        FontAtlas atlas = it->spanAtlas[spanI];
        Range range = it->spanRange[spanI];
        View text(it->text.data + range.offset, range.size);

        for (size_t j = 0; j < text.size; j++, charI++)
        {
            uint32_t c = (uint32_t)text.data[j];

            // TODO: text wrapping using whitespace as boundary
            if (c == '\n' || baseline.x >= it->limitWidth)
            {
                baseline.y += it->lineHeight;
                baseline.x = 0.0f;
                continue;
            }

            float advanceX;
            Rect rect;
            atlas.get_baseline_glyph(c, it->fontSizePx, baseline, rect, advanceX);
            bool quit = (it->glyphCB && it->glyphCB(rect, baseline, charI, spanI, user));
            baseline.x += advanceX;

            if (quit)
                return baseline;
        }
    }

    return baseline;
}

} // namespace LD