#pragma once

#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Impulse.h>
#include <LudensEditor/EditorContext/EditorWindow.h>
#include <LudensEditor/EditorWidget/EUIComponent.h>

namespace LD {

/// @brief Editor inspector window implementation.
struct InspectorWindowObj : EditorWindowObj
{
    InspectorWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info)
    {
    }

    void update(float delta);
};

} // namespace LD