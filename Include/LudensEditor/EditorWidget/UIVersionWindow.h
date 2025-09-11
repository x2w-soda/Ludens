#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UIContext.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

struct UIVersionWindowInfo
{
    UIContext context;
    EditorTheme theme;
};

/// @brief Window to display framework version.
struct UIVersionWindow : Handle<struct UIVersionWindowObj>
{
    static UIVersionWindow create(const UIVersionWindowInfo& info);
    static void destroy(UIVersionWindow window);

    UIWindow get_handle();
};

} // namespace LD