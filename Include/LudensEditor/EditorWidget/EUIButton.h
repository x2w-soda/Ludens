#pragma once

#include <Ludens/UI/Widget/UIButtonWidget.h>
#include <Ludens/UI/Widget/UIImageWidget.h>
#include <Ludens/UI/Widget/UITextWidget.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>

namespace LD {

class EUIButton
{
public:
    bool update(EditorIcon icon, const char* label, const UILayoutInfo* info = nullptr);

private:
    UIImageData mIcon;
    UITextData mText;
    UIButtonData mButton;
};

} // namespace LD