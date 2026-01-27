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

} // namespace LD