#include "Core/UI/Include/Control/UIButton.h"

namespace LD
{

UIButton::UIButton() : UIWidget(UIType::Button)
{
}

UIButton::~UIButton()
{
    UIWidget::Cleanup();
}

void UIButton::Startup(const UIButtonInfo& info)
{
    LD_DEBUG_ASSERT(info.Text.Font && info.Text.Size > 0.0f);

    UIWidget::Startup(info.Widget);
    UIContext* ctx = mWindow->GetContext();
    UITheme* theme = ctx->GetTheme();

    theme->GetPrimaryColor(mBGColor);
    theme->GetOnPrimaryColor(mFGColor);

    // TODO:
    float margin = 4.0f;
    mLayout.SetMargin(margin);

    mFlags |= (IS_HOVERABLE_BIT | IS_PRESSABLE_BIT);
    mLibCallback.OnPress = &UIButton::OnPress;
    mLibCallback.OnRelease = &UIButton::OnRelease;

    mText.Font = info.Text.Font;
    mText.Size = info.Text.Size;
    SetText(info.Text.Content);

    mOnClick = info.OnClick;
    mClickMode = info.ClickMode;
}

void UIButton::Cleanup()
{
    UIWidget::Cleanup();

    mText.Font = nullptr;
    mOnClick = nullptr;
    mUserCallback.Reset();
    mLibCallback.Reset();
}

float UIButton::GetGlyphScale()
{
    return mText.GetGlyphScale();
}

UIFont* UIButton::GetFont()
{
    return mText.Font;
}

View<FontGlyphExt> UIButton::GetTextGlyphs()
{
    return mTextGlyphs.GetView();
}

void UIButton::GetColors(Vec4& bg, Vec4& fg) const
{
    float mix = 0.0f;
    
    if (mFlags & IS_PRESSED_BIT)
        mix = 0.10f;
    else if (mFlags & IS_HOVERED_BIT)
        mix = 0.04f;

    bg = Vec4::Lerp(mBGColor, mFGColor, mix);
    fg = mFGColor;
}

void UIButton::SetText(const UIString& text)
{
    mText.Content = text;
    mTextGlyphs.Resize(mText.Content.Size());

    Vec2 textSize;
    float scale = GetGlyphScale();

    mText.Font->DeriveTextSize(mText.Content, scale, textSize, mTextGlyphs.Data());

    mLayout.SetSize(textSize);
}

void UIButton::OnPress(UIContext* ctx, UIWidget* widget)
{
    UIButton* button = static_cast<UIButton*>(widget);

    if (button->mClickMode == UIClickMode::ClickOnPress && button->mOnClick)
    {
        button->mOnClick(ctx, button);
    }
}

void UIButton::OnRelease(UIContext* ctx, UIWidget* widget)
{
    UIButton* button = static_cast<UIButton*>(widget);

    if (button->mClickMode == UIClickMode::ClickOnRelease && button->mOnClick)
    {
        button->mOnClick(ctx, button);
    }
}

} // namespace LD