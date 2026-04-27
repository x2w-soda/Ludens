#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UIFont.h>
#include <Ludens/UI/UILayer.h>
#include <LudensEditor/EditorContext/EditorContext.h>

namespace LD {
#if 0

struct EditorUIDialogInfo
{
    EditorContext ctx;
    UIFont font;
};

/// @brief Manages all Editor dialogs, these are OS-level Windows
///        under the WindowRegistry.
struct EditorUIDialog : Handle<struct EditorUIDialogObj>
{
    static EditorUIDialog create(const EditorUIDialogInfo& modalI);
    static void destroy(EditorUIDialog modal);

    void render(ScreenRenderComponent renderer);
    void update(float delta);
    WindowID get_dialog_window_id();
};

#endif
} // namespace LD