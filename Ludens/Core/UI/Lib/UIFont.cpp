#include "Core/UI/Include/UI.h"
#include "Core/UI/Include/UIWidget.h"

namespace LD {

UIFont::UIFont()
{
}

UIFont::~UIFont()
{
}

void UIFont::Startup(const UIFontInfo& info)
{
    mTTF = info.TTF;
    mGlyphTable = info.GlyphTable;
}

void UIFont::Cleanup()
{
    mTTF = nullptr;
    mGlyphTable = nullptr;
}

Ref<FontTTF> UIFont::GetTTF()
{
    LD_DEBUG_ASSERT(mTTF);
    return mTTF;
}

Ref<FontGlyphTable> UIFont::GetGlyphTable()
{
    LD_DEBUG_ASSERT(mGlyphTable);
    return mGlyphTable;
}

bool UIFont::DeriveTextSize(const UIString& text, float ratio, Vec2& size, FontGlyphExt* glyphsExt)
{
    int lineSpace;
    mTTF->GetVerticalMetrics(nullptr, nullptr, nullptr, &lineSpace);

    size.x = 0.0f;
    size.y = (float)lineSpace * ratio;

    for (size_t i = 0; i < text.Size(); i++)
    {
        FontGlyphExt& glyph = glyphsExt[i];
        if (!mGlyphTable->GetGlyph((u32)text[i], glyph))
            return false;

        // glyph offset from baseline left most point
        glyph.Offset.x = size.x;
        glyph.Offset.y = 0.0f;
        
        size.x += glyph.AdvanceX * ratio;
    }

    return true;
}

bool UIFont::DeriveTextSizeLimitWidth(const UIString& text, float ratio, float limitWidth, float& height, FontGlyphExt* glyphsExt)
{
    float width = 0.0f;
    int lineSpace;
    mTTF->GetVerticalMetrics(nullptr, nullptr, nullptr, &lineSpace);

    height = 0.0f;

    for (size_t i = 0; i < text.Size(); i++)
    {
        FontGlyphExt& glyph = glyphsExt[i];
        if (!mGlyphTable->GetGlyph((u32)text[i], glyph))
            return false;

        if (width + glyph.AdvanceX * ratio >= limitWidth)
        {
            height += (float)lineSpace * ratio;
            width = 0.0f;
        }

        // glyph offset from top baseline, left most point
        glyph.Offset.y = height;
        glyph.Offset.x = width;

        width += glyph.AdvanceX * ratio;
    }

    height += (float)lineSpace * ratio;
    return false;
}

} // namespace LD