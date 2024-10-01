#pragma once

#include "Core/UI/Include/UI.h"

namespace LD {

class UIButton;

struct UILabelInfo
{
    UIWidgetInfo Widget;
    UIText Text;
};

class UILabel : public UIWidget
{
public:
    UILabel();
    UILabel(const UILabel&) = delete;
    ~UILabel();

    UILabel& operator=(const UILabel&) = delete;

    void Startup(const UILabelInfo& info);
    void Cleanup();

    /// get the text content
    UIString GetText();

    void GetColors(Vec4& bg, Vec4& fg) const;

    /// set the text content
    void SetText(const UIString& text);

    /// get the text size
    float GetTextSize();

    /// set the text size
    void SetTextSize(float size);

    /// get a view to the glyphs for each character in text,
    /// the view is invalidated between calls to SetText().
    View<FontGlyphExt> GetTextGlyphs();

    /// get the ratio of displayed pixel size to the actual glyph size
    float GetGlyphScale();

    UIFont* GetFont();

private:
    Vector<FontGlyphExt> mTextGlyphs;
    Vec4 mBGColor, mFGColor;
    UIText mText;
    float mLimitWidth;
};

} // namespace LD