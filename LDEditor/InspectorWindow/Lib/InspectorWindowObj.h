#pragma once

#include <Ludens/DataRegistry/DataComponent.h>
#include <LudensEditor/EditorContext/EditorCallback.h>
#include <LudensEditor/EditorContext/EditorWindowObj.h>

namespace LD {

/// @brief Editor inspector window implementation.
struct EInspectorWindowObj : EditorWindowObj
{
    virtual ~EInspectorWindowObj() = default;

    CUID subjectID = 0; // subject component being inspected
    ECBSelectAssetFn selectAssetFn;
    void* user;
    bool isSelectingNewAsset = false;

    void inspect_component(CUID compID);

    virtual void on_imgui() override;

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
    static void on_editor_context_event(const EditorContextEvent* event, void* user);
};

} // namespace LD