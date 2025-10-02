#include <Ludens/System/Memory.h>
#include <LudensEditor/EditorWidget/UIAssetSlotWidget.h>

namespace LD {

/// @brief Asset selection widget implementation.
struct UIAssetSlotWidgetObj
{
    AssetType type;
    AUID assetID;
    EditorTheme theme;
    UIPanelWidget rootW;
    UITextWidget textW;
    UITextWidget assetTextW;
    void (*requestAssetFn)(AssetType type, AUID currentID, void* user);
    void* user;

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
    obj->assetID = info.assetID;
    obj->requestAssetFn = info.requestAssetFn;
    obj->user = info.user;

    UIWidget parent = info.parent;
    UINode parentNode = parent.node();
    float fontSize = info.theme.get_font_size();

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_X;
    layoutI.childGap = 10.0f;
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
    textWI.cstr = info.assetName;
    textWI.bgColor = &color;
    obj->assetTextW = rootNode.add_text({}, textWI, obj);
    obj->assetTextW.set_on_mouse([](UIWidget widget, const Vec2& pos, MouseButton btn, UIEvent event) {
        UIAssetSlotWidgetObj& self = *(UIAssetSlotWidgetObj*)widget.get_user();

        if (btn != MOUSE_BUTTON_LEFT || event != UI_MOUSE_DOWN)
            return;

        if (self.requestAssetFn && self.assetID)
            self.requestAssetFn(self.type, self.assetID, self.user);
    });

    return UIAssetSlotWidget(obj);
}

void UIAssetSlotWidget::destroy(UIAssetSlotWidget widget)
{
    UIAssetSlotWidgetObj* obj = widget.unwrap();

    heap_delete<UIAssetSlotWidgetObj>(obj);
}

void UIAssetSlotWidget::set_asset(AUID assetID, const char* assetName)
{
    mObj->assetID = assetID;
    mObj->assetTextW.set_text(assetName);
}

AssetType UIAssetSlotWidget::get_type()
{
    return mObj->type;
}

AUID UIAssetSlotWidget::get_id()
{
    return mObj->assetID;
}

void UIAssetSlotWidget::show()
{
    mObj->rootW.show();
}

void UIAssetSlotWidget::hide()
{
    mObj->rootW.hide();
}

} // namespace LD