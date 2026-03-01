#pragma once

#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Impulse.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

/// @brief Editor inspector window implementation.
struct InspectorWindowObj : EditorWindowObj
{
    SUID subjectSUID = 0; // subject component being inspected
    Impulse isRequestingNewAsset{};
    AssetType requestAssetType{};
    AssetID oldAssetID = 0;
    EditorContext ctx{};

    InspectorWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info), ctx(info.ctx)
    {
        mCtx.add_observer(&InspectorWindowObj::on_editor_event, this);
    }

    static void on_editor_event(const EditorEvent* event, void* user);

    virtual EditorWindowType get_type() override { return EDITOR_WINDOW_INSPECTOR; }
    virtual void on_imgui(float delta) override;
    void request_new_asset(AssetType type, AssetID currentID);
};

} // namespace LD