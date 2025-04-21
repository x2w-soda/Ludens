#include <Ludens/Header/Assert.h>
#include <Ludens/Media/Font.h>
#include <Ludens/Media/Bitmap.h>
#include <Ludens/System/Memory.h>
#include <msdf-atlas-gen/msdf-atlas-gen.h> // hide from user
#include <vector>

namespace LD {

static msdfgen::FreetypeHandle* msdfFreeType;

struct FontObj
{
    msdfgen::FontHandle* msdfHandle;
};

struct FontAtlasObj
{
    Font font;
    Bitmap atlas;
    std::vector<msdf_atlas::GlyphGeometry> glyphs;
};

static Bitmap generate_sdf_atlas(FontObj* obj, std::vector<msdf_atlas::GlyphGeometry>& glyphs);

Font Font::create_from_path(const char* path)
{
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

void Font::destroy(Font font)
{
    FontObj* obj = (FontObj*)font;

    msdfgen::destroyFont(obj->msdfHandle);

    heap_delete<FontObj>(obj);
}

static Bitmap generate_sdf_atlas(FontObj* obj, std::vector<msdf_atlas::GlyphGeometry>& glyphs)
{
    msdfgen::FontHandle* font = obj->msdfHandle;

    msdf_atlas::FontGeometry fontGeometry(&glyphs);
    fontGeometry.loadCharset(font, 1.0f, msdf_atlas::Charset::ASCII);

    const double maxCornerAngle = 3.0f;
    for (msdf_atlas::GlyphGeometry& glyph : glyphs)
        glyph.edgeColoring(&msdfgen::edgeColoringInkTrap, maxCornerAngle, 0);

    msdf_atlas::GridAtlasPacker packer;

    packer.setDimensionsConstraint(msdf_atlas::DimensionsConstraint::SQUARE);
    packer.setMinimumScale(36.0);
    packer.setPixelRange(2.0);
    packer.setMiterLimit(1.0);
    packer.setSpacing(4);
    packer.pack(glyphs.data(), (int)glyphs.size());

    int atlasWidth, atlasHeight;
    packer.getDimensions(atlasWidth, atlasHeight);
    msdf_atlas::ImmediateAtlasGenerator<
        float, 1,                 // number of atlas color channels
        msdf_atlas::sdfGenerator, // function to generate bitmaps for individual glyphs
        msdf_atlas::BitmapAtlasStorage<msdfgen::byte, 1>>
        generator(atlasWidth, atlasHeight);

    generator.setAttributes({});
    generator.setThreadCount(4);
    generator.generate(glyphs.data(), (int)glyphs.size());

    const msdfgen::BitmapConstRef<msdfgen::byte, 1>& bitmap = generator.atlasStorage();
    return Bitmap::create_from_data(bitmap.width, bitmap.height, BITMAP_CHANNEL_R, bitmap.pixels);
}

FontAtlas FontAtlas::create_sdf(Font font)
{
    FontAtlasObj* obj = heap_new<FontAtlasObj>(MEMORY_USAGE_MEDIA);

    // TODO: configuration
    obj->font = font;
    obj->atlas = generate_sdf_atlas(font, obj->glyphs);

    return {obj};
}

void FontAtlas::destroy(FontAtlas atlas)
{
    FontAtlasObj* obj = (FontAtlasObj*)atlas;

    if (obj->atlas)
        Bitmap::destroy(obj->atlas);

    heap_delete<FontAtlasObj>(obj);
}

Bitmap FontAtlas::get_bitmap()
{
    return mObj->atlas;
}

bool FontAtlas::get_glyph(uint32_t code, int& x, int& y, int& w, int& h)
{
    if (code > (uint32_t)mObj->glyphs.size())
        return false;

    LD_ASSERT(code < 128);

    const msdf_atlas::GlyphGeometry* geo = mObj->glyphs.data() + (code - 32);
    geo->getBoxRect(x, y, w, h);

    return true;
}

} // namespace LD