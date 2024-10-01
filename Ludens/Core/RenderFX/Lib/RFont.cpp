#include <stb/stb_rect_pack.h>
#include <stb/stb_truetype.h>
#include "Core/DSA/Include/Vector.h"
#include "Core/RenderBase/Include/RDevice.h"
#include "Core/RenderBase/Include/RTexture.h"
#include "Core/RenderFX/Include/RFont.h"

namespace LD {

struct FontRange
{
    void Set(u32 start, size_t charCount, float fontSize)
    {
        PackedChars.Resize(charCount);
        PackRange.font_size = fontSize;
        PackRange.num_chars = charCount;
        PackRange.first_unicode_codepoint_in_range = start;
        PackRange.chardata_for_range = PackedChars.Data();
        PackRange.array_of_unicode_codepoints = nullptr;
    }

    stbtt_pack_range PackRange;
    Vector<stbtt_packedchar> PackedChars;
};

// intermediate data when creating font atlas
struct FontBuildData
{
    stbtt_pack_context PackContext;
    Vector<FontRange> Ranges;
};

RFontAtlas::~RFontAtlas()
{
    LD_DEBUG_ASSERT(!mDevice);
}

void RFontAtlas::Startup(const RFontAtlasInfo& info)
{
    mDevice = info.Device;
    float pixelSize = info.FontData->GetPixelSize();
    auto ttfData = static_cast<const unsigned char*>(info.FontData->GetData());

    FontBuildData bd;
    int fontOffset = stbtt_GetFontOffsetForIndex(ttfData, 0);

    mAtlasWidth = 2048;
    mAtlasHeight = 2048;
    u8* grayscale = (u8*)MemoryAlloc(mAtlasWidth * mAtlasHeight);
    u8* rgba = (u8*)MemoryAlloc(mAtlasWidth * mAtlasHeight * 4);

    bd.Ranges.Resize(1);
    FontRange& asciiRange = bd.Ranges.Back();
    
    stbtt_PackBegin(&bd.PackContext, grayscale, mAtlasWidth, mAtlasHeight, 0, 1, NULL);
    {
        asciiRange.Set(32, 96, pixelSize);

        int result = stbtt_PackFontRanges(&bd.PackContext, ttfData, fontOffset, &asciiRange.PackRange, 1);
        LD_DEBUG_ASSERT(result != 0);
    }
    stbtt_PackEnd(&bd.PackContext);

    // NOTE: here we extend single channel information to RGBA, this is due to
    //       how the Rect batching shader is designed.

    for (size_t i = 0; i < mAtlasWidth * mAtlasHeight; i++)
    {
        rgba[4 * i + 0] = grayscale[i];
        rgba[4 * i + 1] = grayscale[i];
        rgba[4 * i + 2] = grayscale[i];
        rgba[4 * i + 3] = grayscale[i];
    }

    RTextureInfo atlasInfo{};
    atlasInfo.Format = RTextureFormat::RGBA8;
    atlasInfo.Width = mAtlasWidth;
    atlasInfo.Height = mAtlasHeight;
    atlasInfo.Type = RTextureType::Texture2D;
    atlasInfo.Data = rgba;
    atlasInfo.Size = mAtlasWidth * mAtlasHeight * 4;
    mDevice.CreateTexture(mAtlas, atlasInfo);

    MemoryFree(rgba);
    MemoryFree(grayscale);

    // create glyph lookup table
    u32 start = asciiRange.PackRange.first_unicode_codepoint_in_range;
    mGlyphTable = MakeRef<FontGlyphTable>();
    for (size_t i = 0; i < asciiRange.PackedChars.Size(); i++)
    {
        u32 codepoint = start + i;
        const stbtt_packedchar& pc = asciiRange.PackedChars[i];

        FontGlyph glyph;
        glyph.Codepoint = codepoint;
        glyph.AdvanceX = std::abs(pc.xadvance);
        glyph.BearingX = std::abs(pc.xoff);
        glyph.BearingY = std::abs(pc.yoff);
        glyph.RectXY = { { (float)pc.x0, (float)pc.y0 }, { (float)pc.x1, (float)pc.y1 } };
        glyph.RectUV = { { (float)pc.x0 / mAtlasWidth, (float)pc.y0 / mAtlasHeight },
                         { (float)pc.x1 / mAtlasWidth, (float)pc.y1 / mAtlasHeight } };

        mGlyphTable->Glyphs[codepoint] = glyph;
    }
}

void RFontAtlas::Cleanup()
{
    mDevice.DeleteTexture(mAtlas);
    mDevice.ResetHandle();
    mGlyphTable = nullptr;
}

RTexture RFontAtlas::GetAtlas()
{
    LD_DEBUG_ASSERT(mAtlas);
    return mAtlas;
}

} // namespace LD