#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UIWindow.h>
#include <Ludens/UI/UIWindowManager.h>
#include <LudensEditor/EditorContext/EditorCallback.h>
#include <LudensEditor/EditorContext/EditorContext.h>

namespace LD {

struct EInspectorWindowInfo
{
    EditorContext ctx;              /// editor context handle
    UIWindowManager wm;             /// window manager handle
    UIWMAreaID areaID;              /// designated window area
    ECBSelectAssetFn selectAssetFn; /// dependency injection to select an asset
    void* user;                     /// used in callbacks
};

/// @brief Editor inspector window.
///        Displays the properties of the selected object.
struct EInspectorWindow : Handle<struct EInspectorWindowObj>
{
    /// @brief Create editor inspector window.
    /// @param windowInfo Inspector window creation window.
    /// @return Editor inspector window handle.
    static EInspectorWindow create(const EInspectorWindowInfo& windowInfo);

    /// @brief Destroy editor inspector window.
    static void destroy(EInspectorWindow window);

    /// @brief If an UIAssetSlotWidget is requesting a new asset,
    ///        this propagates the new asset ID to the widget.
    void select_asset(AUID assetID);
};

} // namespace LD