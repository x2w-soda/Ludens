#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/RenderComponent/ScreenRenderComponent.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

struct EditorDialogInfo
{
    EditorContext ctx;     // connection with editor context
    EditorWindowType type; // the type to host in this dialog
    Vec2 extent;           // editor dialog window extent
    FontAtlas fontAtlas;   // font atlas used to render text
    RImage fontAtlasImage; // font atlas image handle
};

struct EditorDialog : Handle<struct EditorDialogObj>
{
    static EditorDialog create(const EditorDialogInfo& dialogI);
    static void destroy(EditorDialog dialog);

    void update(float delta);
    void render(ScreenRenderComponent renderer);

    ///@brief Hint that user should destroy this dialog.
    bool should_close();

    EditorWindow get_editor_window(EditorWindowType typeCheck);
    WindowID get_id();
};

} // namespace LD