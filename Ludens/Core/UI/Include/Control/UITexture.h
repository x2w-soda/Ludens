#pragma once

#include "Core/UI/Include/UIWidget.h"

namespace LD {

struct UITextureInfo
{
    UIWidgetInfo Widget;
    UID TextureID;
};

class UITexture : public UIWidget
{
public:
    UITexture();
    UITexture(const UITexture&) = delete;
    ~UITexture();

    UITexture& operator=(const UITexture&) = delete;

    void Startup(const UITextureInfo& info);
    void Cleanup();

    UID GetTextureID() const
    {
        return mTextureID;
    }

private:
    UID mTextureID;
};

} // namespace LD