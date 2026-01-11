#include <Ludens/Header/Assert.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorWidget/UIAssetSlotWidget.h>
#include <LudensEditor/EditorWidget/UIDraw.h>

namespace LD {

bool eui_asset_slot(EditorTheme theme, AssetType assetType, AUID assetID, const char* assetName)
{
    MouseButton btn;
    UIEvent event;
    bool newAssetRequest = false;

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = 10.0f;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    ui_push_panel();
    ui_top_layout(layoutI);

    ui_push_text(get_asset_type_cstr(assetType));
    ui_pop();

    LD_ASSERT(assetName);
    ui_push_text(assetName);
    ui_top_draw(&eui_draw_text_with_bg);
    if (ui_top_mouse_down(btn) && btn == MOUSE_BUTTON_LEFT)
        newAssetRequest = true;
    ui_pop();

    ui_pop();

    return newAssetRequest;
}

} // namespace LD
