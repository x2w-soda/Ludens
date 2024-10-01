#pragma once

#include <string>
#include <unordered_map>
#include <stb/stb_truetype.h>
#include "Core/Header/Include/Types.h"
#include "Core/Math/Include/Rect2D.h"

namespace LD {

/// a glyph in a font atlas
struct FontGlyph
{
    /// unicode codepoint
    u32 Codepoint;

    /// horizontal offset to advance the cursor after rendering this glyph
    f32 AdvanceX;

    /// horizontal offset from the cursor to the left edge of glyph bounding box
    f32 BearingX;

    /// vertical offset from the cursor to the top edge of glyph bounding box
    f32 BearingY;

    /// glyph bounding box in the texture atlas
    Rect2D RectXY;

    /// glyph UV calculated from RectXY and texture atlas size
    Rect2D RectUV;
};

/// extended font glyph information for presentation, used by the UI framework
struct FontGlyphExt : FontGlyph
{
    /// offset from some origin reference point
    Vec2 Offset;
};

/// font glyph lookup table
struct FontGlyphTable
{
    std::unordered_map<u32, FontGlyph> Glyphs;

    /// @brief lookup font glyph from unicode
    /// @param code unicode
    /// @param glyph output glyph, if found
    /// @return true if the unicode glyph is found in Glyphs, false otherwise
    inline bool GetGlyph(u32 code, FontGlyph& glyph)
    {
        if (Glyphs.find(code) == Glyphs.end())
            return false;

        glyph = Glyphs[code];
        return true;
    }
};

struct FontTTFInfo
{
    std::string Name;
    const void* TTFData;
    size_t TTFSize;
    float PixelSize;
};

/// encapsulates font data from a TTF file
class FontTTF
{
public:
    FontTTF() = delete;

    /// the TTF data is copied and freed upon destructor
    FontTTF(const FontTTFInfo& info);
    ~FontTTF();

    std::string GetName() const;

    float GetPixelSize() const;


    const void* GetData();

    void GetVerticalMetrics(int* ascent, int* descent, int* lineGap, int* lineSpace);

private:
    void SetPixelSize(float size);

    stbtt_fontinfo mFontInfo;
    void* mTTFData;
    size_t mTTFSize;
    std::string mName;
    float mPixelSize;
    int mAscent;
    int mDescent;
    int mLineGap;
    int mLineSpace;
};

} // namespace LD