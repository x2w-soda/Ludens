#include <Ludens/System/Memory.h>
#include <LudensEditor/EditorWidget/UIAssetSlotWidget.h>

namespace LD {

/// @brief Asset selection widget implementation.
struct UIAssetSlotWidgetObj
{
    AssetType type;
    AUID* asset;
    EditorTheme theme;
    UIPanelWidget rootW;
    UITextWidget textW;
    UITextWidget assetTextW;

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

void UIAssetSlotWidgetObj::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    Rect rect = widget.get_rect();
    renderer.draw_rect(rect, 0xFFFFFFFF);
}

UIAssetSlotWidget UIAssetSlotWidget::create(const UIAssetSlotWidgetInfo& info)
{
    auto* obj = heap_new<UIAssetSlotWidgetObj>(MEMORY_USAGE_UI);
    obj->theme = info.theme;
    obj->type = info.type;
    obj->asset = info.asset;

    UIWidget parent = info.parent;
    UINode parentNode = parent.node();
    float fontSize = info.theme.get_font_size();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    UIPanelWidgetInfo panelWI{};
    obj->rootW = parentNode.add_panel(layoutI, panelWI, obj);

    UINode rootNode = obj->rootW.node();
    UITextWidgetInfo textWI{};
    textWI.hoverHL = false;
    textWI.fontSize = fontSize;
    textWI.cstr = get_asset_type_cstr(obj->type);
    obj->textW = rootNode.add_text({}, textWI, obj);

    Color color = obj->theme.get_ui_theme().get_field_color();
    textWI.cstr = nullptr;
    textWI.bgColor = &color;
    obj->assetTextW = rootNode.add_text({}, textWI, obj);

    return UIAssetSlotWidget(obj);
}

void UIAssetSlotWidget::destroy(UIAssetSlotWidget widget)
{
    UIAssetSlotWidgetObj* obj = widget.unwrap();

    heap_delete<UIAssetSlotWidgetObj>(obj);
}

void UIAssetSlotWidget::set(AUID* asset)
{
    mObj->asset = asset;
}

void UIAssetSlotWidget::set_asset_name(const char* assetName)
{
    mObj->assetTextW.set_text(assetName);
}

void UIAssetSlotWidget::show()
{
    mObj->rootW.show();
}

void UIAssetSlotWidget::hide()
{
    mObj->rootW.hide();
}

void UIAssetSlotWidget::select()
{
    // TODO:
}

} // namespace LD