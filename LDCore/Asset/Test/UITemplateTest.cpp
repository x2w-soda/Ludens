#include <Extra/doctest/doctest.h>
#include <Ludens/Asset/Template/UITemplateSchema.h>

using namespace LD;

struct UITemplateTest
{
    static bool on_load(UIWidget widget, const UITemplateEntry& tmpl, void* user);
    static bool on_save(UITextWidget widget, const UITemplateEntry& tmpl, void* user);
};

bool UITemplateTest::on_load(UIWidget widget, const UITemplateEntry& tmpl, void* user)
{
    // TODO:
    return true;
}

bool UITemplateTest::on_save(UITextWidget widget, const UITemplateEntry& tmpl, void* user)
{
    // TODO:
    return true;
}

static const char sUIToml[] = R"(
[ludens_ui_template]
version_major = 0
version_minor = 0
version_patch = 0

[hierarchy]
0 = [1]

[[widget]]
index = 0
type = 'UIPanel'
layout = { size_x = 'fit', size_y = 'fit', child_axis = 'x', child_align_x = 'center', child_align_y = 'center', child_gap = 10, child_padding = {left = 0, right = 0, top = 0, bottom = 0}}
color = 0x000000FF

[[widget]]
index = 1
type = 'UIImage'
layout = { size_x = 'fit', size_y = 'fit', child_axis = 'x', child_align_x = 'center', child_align_y = 'center', child_gap = 10, child_padding = {left = 0, right = 0, top = 0, bottom = 0}}
color = 0xFF00FFFF
image_rect = { x = 0, y = 0, w = 10, h = 50 }
texture_2d = 1234
)";

TEST_CASE("UITemplate")
{
    UIThemeInfo themeI = UITheme::get_default_info();
    UIContext ctx = UIContext::create({.theme = UITheme(&themeI)});
    UITemplate tmpl = UITemplate::create();

    std::string err;
    bool ok = UITemplateSchema::load_ui_template_from_source(tmpl, View(sUIToml, sizeof(sUIToml) - 1), err);
    CHECK(ok);

    UIWorkspace space = ctx.create_layer("test").create_workspace(Rect(0, 0, 100, 100));
    UILayoutInfo layoutI{};
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    UIWindowInfo windowI{};
    windowI.name = "test";
    UIWindow window = space.create_window(space.get_root_id(), layoutI, windowI, nullptr);

    UITemplateTest test;
    UIWidget subtree = tmpl.load(window, &UITemplateTest::on_load, &test);

    std::vector<UIWidget> widgets;
    window.get_widgets(widgets);

    CHECK(widgets.size() == 2);
    CHECK(widgets[0].get_type() == UI_WIDGET_PANEL);
    CHECK(widgets[1].get_type() == UI_WIDGET_IMAGE);

    widgets[0].node().get_children(widgets);
    CHECK(widgets.size() == 1);
    CHECK(widgets[0].get_type() == UI_WIDGET_IMAGE);

    UITemplate::destroy(tmpl);
    UIContext::destroy(ctx);
}