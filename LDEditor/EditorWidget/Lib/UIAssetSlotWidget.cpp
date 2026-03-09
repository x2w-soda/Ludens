#include <Ludens/Header/Assert.h>
#include <Ludens/Header/MouseValue.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorWidget/UIAssetSlotWidget.h>
#include <LudensEditor/EditorWidget/UIDraw.h>

namespace LD {

bool eui_asset_slot(EditorTheme theme, AssetType assetType, AssetID assetID, const char* assetName)
{
    Vec2 mousePos;
    MouseValue mouseVal;
    UIEvent event;
    bool newAssetRequest = false;
    float childGap = 6.0f;

    UILayoutInfo layoutI = theme.make_hbox_layout(&childGap);
    layoutI.sizeX = UISize::grow();
    ui_push_panel();
    ui_top_layout(layoutI);

    layoutI = theme.make_text_label_layout();
    ui_push_text(get_asset_type_cstr(assetType));
    ui_top_layout(layoutI);
    ui_pop();

    if (!assetName)
        assetName = "N/A";

    ui_push_text(assetName);
    ui_top_layout(layoutI);
    ui_top_draw(&eui_draw_text_with_bg);
    if (ui_top_mouse_down(mouseVal, mousePos) && mouseVal.button() == MOUSE_BUTTON_LEFT)
        newAssetRequest = true;
    ui_pop();

    ui_pop();

    return newAssetRequest;
}

} // namespace LD
