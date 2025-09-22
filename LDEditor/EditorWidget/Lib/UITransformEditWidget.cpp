#include <Ludens/Header/Math/Transform.h>
#include <LudensEditor/EditorWidget/UITransformEditWidget.h>
#include <format>

namespace LD {

/// @brief Transform edit widget implementation.
struct UITransformEditWidgetObj
{
    UIPanelWidget root;
    EditorTheme theme;
    Transform* subject;
    float panelChildGap;

    struct Row
    {
        UITransformEditWidgetObj* transformEdit = nullptr;
        UIPanelWidget panel;
        UITextWidget label;
        UITextWidget x;
        UITextWidget y;
        UITextWidget z;

        void startup(UITransformEditWidgetObj* obj, UINode parentNode, float panelChildGap, float fontSize, const char* rowLabel)
        {
            transformEdit = obj;

            UILayoutInfo layoutI{};
            layoutI.childAxis = UI_AXIS_X;
            layoutI.childPadding = {2.0f, 2.0f, 2.0f, 2.0f};
            layoutI.childGap = panelChildGap;
            layoutI.sizeX = UISize::grow();
            layoutI.sizeY = UISize::fit();
            UIPanelWidgetInfo panelWI{};
            panel = parentNode.add_panel(layoutI, panelWI, this);
            panel.set_on_draw(&Row::on_draw);

            UITextWidgetInfo textWI{};
            textWI.cstr = nullptr;
            textWI.hoverHL = false;
            textWI.fontSize = fontSize;

            // TODO: UITextEditWidget, currently we are only displaying transform values
            UINode panelN = panel.node();
            layoutI.sizeX = UISize::fixed(100);
            layoutI.sizeY = UISize::fixed(textWI.fontSize * 1.2f);
            label = panelN.add_text(layoutI, textWI, nullptr);
            label.set_text(rowLabel);
            x = panelN.add_text(layoutI, textWI, nullptr);
            y = panelN.add_text(layoutI, textWI, nullptr);
            z = panelN.add_text(layoutI, textWI, nullptr);
        }

        static void on_draw(UIWidget widget, ScreenRenderComponent renderer)
        {
            Row& self = *(Row*)widget.get_user();
            if (!self.transformEdit->subject)
                return;

            float panelChildGap = self.transformEdit->panelChildGap;

            Color colorX, colorY, colorZ;
            self.transformEdit->theme.get_gizmo_colors(colorX, colorY, colorZ);
            UITheme theme = self.transformEdit->theme.get_ui_theme();

            Rect rect = self.label.get_rect();
            Vec2 pos = rect.get_pos_tr();

            renderer.draw_rect(Rect(pos.x, pos.y, panelChildGap, rect.h), colorX);
            pos.x += self.x.get_rect().w + panelChildGap;

            renderer.draw_rect(Rect(pos.x, pos.y, panelChildGap, rect.h), colorY);
            pos.x += self.y.get_rect().w + panelChildGap;

            renderer.draw_rect(Rect(pos.x, pos.y, panelChildGap, rect.h), colorZ);

            renderer.draw_rect(self.x.get_rect(), theme.get_field_color());
            renderer.draw_rect(self.y.get_rect(), theme.get_field_color());
            renderer.draw_rect(self.z.get_rect(), theme.get_field_color());
        }
    } position, rotation, scale;

    static void on_update(UIWidget widget, float delta);
};

void UITransformEditWidgetObj::on_update(UIWidget widget, float delta)
{
    auto& self = *(UITransformEditWidgetObj*)widget.get_user();

    if (!self.subject)
        return;

    const Transform& T = *self.subject;
    std::string str;

    str = std::format("{:8.3f}", T.position.x);
    self.position.x.set_text(str.c_str());
    str = std::format("{:8.3f}", T.position.y);
    self.position.y.set_text(str.c_str());
    str = std::format("{:8.3f}", T.position.z);
    self.position.z.set_text(str.c_str());

    str = std::format("{:8.3f}", T.rotation.x);
    self.rotation.x.set_text(str.c_str());
    str = std::format("{:8.3f}", T.rotation.y);
    self.rotation.y.set_text(str.c_str());
    str = std::format("{:8.3f}", T.rotation.z);
    self.rotation.z.set_text(str.c_str());

    str = std::format("{:8.3f}", T.scale.x);
    self.scale.x.set_text(str.c_str());
    str = std::format("{:8.3f}", T.scale.y);
    self.scale.y.set_text(str.c_str());
    str = std::format("{:8.3f}", T.scale.z);
    self.scale.z.set_text(str.c_str());
}

UITransformEditWidget UITransformEditWidget::create(const UITransformEditWidgetInfo& info)
{
    EditorTheme editorTheme = info.theme;
    UIWidget parent = info.parent;
    UITheme theme = parent.get_theme();

    auto* obj = heap_new<UITransformEditWidgetObj>(MEMORY_USAGE_UI);
    obj->theme = editorTheme;
    obj->subject = nullptr;
    const float panelChildGap = 6.0f;
    obj->panelChildGap = panelChildGap;

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    UIPanelWidgetInfo panelWI{};
    obj->root = parent.node().add_panel(layoutI, panelWI, obj);
    obj->root.set_on_draw([](UIWidget, ScreenRenderComponent) {}); // dont draw panel
    UINode rootN = obj->root.node();

    float fontSize = editorTheme.get_font_size();

    obj->position.startup(obj, rootN, panelChildGap, fontSize, "Position");
    obj->rotation.startup(obj, rootN, panelChildGap, fontSize, "Rotation");
    obj->scale.startup(obj, rootN, panelChildGap, fontSize, "Scale");
    obj->root.set_on_update(&UITransformEditWidgetObj::on_update);

    return {obj};
}

void UITransformEditWidget::destroy(UITransformEditWidget widget)
{
    UITransformEditWidgetObj* obj = widget.unwrap();

    heap_delete<UITransformEditWidgetObj>(obj);
}

void UITransformEditWidget::set(Transform* transform)
{
    mObj->subject = transform;
}

void UITransformEditWidget::show()
{
    mObj->root.show();
}

void UITransformEditWidget::hide()
{
    mObj->root.hide();
}

} // namespace LD
