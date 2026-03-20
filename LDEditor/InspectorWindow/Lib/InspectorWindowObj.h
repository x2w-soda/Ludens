#pragma once

#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Impulse.h>
#include <LudensEditor/EditorContext/EditorWindow.h>
#include <LudensEditor/EditorWidget/EUIComponent.h>

namespace LD {

/// @brief Editor inspector window implementation.
struct InspectorWindowObj : EditorWindowObj
{
    EditorContext ctx{};
    EUIComponentStorage storage{};

    InspectorWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info), ctx(info.ctx)
    {
    }

    virtual EditorWindowType get_type() override { return EDITOR_WINDOW_INSPECTOR; }
    virtual void on_imgui(float delta) override;
};

} // namespace LD