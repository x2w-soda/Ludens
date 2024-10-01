#pragma once

#include "Core/OS/Include/Memory.h"
#include "Core/Header/Include/Types.h"
#include "Core/Math/Include/Rect2D.h"
#include "Core/RenderBase/Include/RTexture.h"
#include "Core/RenderBase/Include/RDevice.h"
#include "Core/Media/Include/Font.h"

namespace LD {

struct RFontAtlasInfo
{
    RDevice Device;
    Ref<FontTTF> FontData;
};

class RFontAtlas
{
public:
    RFontAtlas() = default;
    RFontAtlas(const RFontAtlas&) = delete;
    ~RFontAtlas();

    RFontAtlas& operator=(const RFontAtlas&) = delete;

    void Startup(const RFontAtlasInfo& info);
    void Cleanup();

    RTexture GetAtlas();

    inline bool GetGlyph(u32 code, FontGlyph& glyph)
    {
        LD_DEBUG_ASSERT(mGlyphTable);
        return mGlyphTable->GetGlyph(code, glyph);
    }

    inline Ref<FontGlyphTable> GetGlyphTable()
    {
        LD_DEBUG_ASSERT(mGlyphTable);
        return mGlyphTable;
    }

private:
    RDevice mDevice;
    RTexture mAtlas;
    int mAtlasWidth;
    int mAtlasHeight;
    Ref<FontGlyphTable> mGlyphTable;
};

} // namespace LD