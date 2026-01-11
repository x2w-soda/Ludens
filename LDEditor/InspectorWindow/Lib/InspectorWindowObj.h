#pragma once

#include <Ludens/DataRegistry/DataComponent.h>
#include <Ludens/Header/Impulse.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

/// @brief Editor inspector window implementation.
struct InspectorWindowObj : EditorWindowObj
{
    EditorContext ctx;
    UIWorkspace space;
    UIWindow root;
    CUID subjectID = 0; // subject component being inspected
    Impulse isRequestingNewAsset;
    AssetType requestAssetType;
    AUID oldAssetID;

    static void on_editor_context_event(const EditorContextEvent* event, void* user);

    virtual EditorWindowType get_type() override { return EDITOR_WINDOW_INSPECTOR; }
    virtual void on_imgui(float delta) override;
    void request_new_asset(AssetType type, AUID currentID);
};

} // namespace LD