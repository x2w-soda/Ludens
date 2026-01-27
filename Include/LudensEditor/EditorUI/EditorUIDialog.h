#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UILayer.h>
#include <LudensEditor/EditorContext/EditorContext.h>

namespace LD {

struct EditorUIDialogInfo
{
    EditorContext ctx;
    FontAtlas fontAtlas;
    RImage fontAtlasImage;
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

} // namespace LD