#include "Core/UI/Include/Control/UITexture.h"

namespace LD {

UITexture::UITexture() : UIWidget(UIType::Texture)
{
}

UITexture::~UITexture()
{
    UIWidget::Cleanup();
}

void UITexture::Startup(const UITextureInfo& info)
{
    UIWidget::Startup(info.Widget);

    mTextureID = info.TextureID;
}

void UITexture::Cleanup()
{
    UIWidget::Cleanup();
}

} // namespace LD