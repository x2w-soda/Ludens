#include <Ludens/Header/Math/Transform.h>
#include <LudensEditor/EditorContext/EditorWidget.h>
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
        UIPanelWidget panel;
        UITextWidget label;
        UITextWidget x;
        UITextWidget y;
        UITextWidget z;

        void on_draw(EditorTheme theme, float panelChildGap, ScreenRenderComponent renderer)
        {
            Color colorX, colorY, colorZ;
            theme.get_gizmo_colors(colorX, colorY, colorZ);

            label.on_draw(renderer);

            Rect rect = label.get_rect();
            Vec2 pos = rect.get_pos_tr();

            renderer.draw_rect(Rect(pos.x, pos.y, panelChildGap, rect.h), colorX);
            x.on_draw(renderer);
            pos.x += x.get_rect().w + panelChildGap;

            renderer.draw_rect(Rect(pos.x, pos.y, panelChildGap, rect.h), colorY);
            y.on_draw(renderer);
            pos.x += y.get_rect().w + panelChildGap;

            renderer.draw_rect(Rect(pos.x, pos.y, panelChildGap, rect.h), colorZ);
            z.on_draw(renderer);
        }
    } position, rotation, scale;

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer);
};

void UITransformEditWidgetObj::on_draw(UIWidget widget, ScreenRenderComponent renderer)
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
    self.position.on_draw(self.theme, self.panelChildGap, renderer);

    str = std::format("{:8.3f}", T.rotation.x);
    self.rotation.x.set_text(str.c_str());
    str = std::format("{:8.3f}", T.rotation.y);
    self.rotation.y.set_text(str.c_str());
    str = std::format("{:8.3f}", T.rotation.z);
    self.rotation.z.set_text(str.c_str());
    self.rotation.on_draw(self.theme, self.panelChildGap, renderer);

    str = std::format("{:8.3f}", T.scale.x);
    self.scale.x.set_text(str.c_str());
    str = std::format("{:8.3f}", T.scale.y);
    self.scale.y.set_text(str.c_str());
    str = std::format("{:8.3f}", T.scale.z);
    self.scale.z.set_text(str.c_str());
    self.scale.on_draw(self.theme, self.panelChildGap, renderer);
}

UITransformEditWidget UITransformEditWidget::create(const UITransformEditWidgetInfo& info)
{
    EditorTheme theme = info.theme;
    UIWidget parent = info.parent;

    auto* obj = heap_new<UITransformEditWidgetObj>(MEMORY_USAGE_UI);
    obj->theme = theme;
    obj->subject = nullptr;
    const float panelChildGap = 6.0f;
    obj->panelChildGap = panelChildGap;

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    UIPanelWidgetInfo panelWI{};
    theme.get_background_color(panelWI.color);
    obj->root = parent.node().add_panel(layoutI, panelWI, obj);
    UINode rootN = obj->root.node();

    layoutI.childAxis = UI_AXIS_X;
    layoutI.childPadding = {2.0f, 2.0f, 2.0f, 2.0f};
    layoutI.childGap = panelChildGap;
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::fit();
    obj->position.panel = rootN.add_panel(layoutI, panelWI, nullptr);
    obj->rotation.panel = rootN.add_panel(layoutI, panelWI, nullptr);
    obj->scale.panel = rootN.add_panel(layoutI, panelWI, nullptr);

    UITextWidgetInfo textWI{};
    textWI.cstr = nullptr;
    textWI.hoverHL = false;
    theme.get_font_size(textWI.fontSize);

    // TODO: UITextEditWidget, currently we are only displaying transform values
    UINode panelN = obj->position.panel.node();
    layoutI.sizeX = UISize::fixed(100);
    layoutI.sizeY = UISize::fixed(textWI.fontSize);
    obj->position.label = panelN.add_text(layoutI, textWI, nullptr);
    obj->position.label.set_text("Position");
    obj->position.x = panelN.add_text(layoutI, textWI, nullptr);
    obj->position.y = panelN.add_text(layoutI, textWI, nullptr);
    obj->position.z = panelN.add_text(layoutI, textWI, nullptr);

    panelN = obj->rotation.panel.node();
    obj->rotation.label = panelN.add_text(layoutI, textWI, nullptr);
    obj->rotation.label.set_text("Rotation");
    obj->rotation.x = panelN.add_text(layoutI, textWI, nullptr);
    obj->rotation.y = panelN.add_text(layoutI, textWI, nullptr);
    obj->rotation.z = panelN.add_text(layoutI, textWI, nullptr);

    panelN = obj->scale.panel.node();
    obj->scale.label = panelN.add_text(layoutI, textWI, nullptr);
    obj->scale.label.set_text("Scale");
    obj->scale.x = panelN.add_text(layoutI, textWI, nullptr);
    obj->scale.y = panelN.add_text(layoutI, textWI, nullptr);
    obj->scale.z = panelN.add_text(layoutI, textWI, nullptr);

    obj->root.set_on_draw(&UITransformEditWidgetObj::on_draw);

    return {obj};
}

void UITransformEditWidget::set(Transform* transform)
{
    mObj->subject = transform;
}

void UITransformEditWidget::on_draw(ScreenRenderComponent renderer)
{
    mObj->root.on_draw(renderer);
}

} // namespace LD
