#include "EditorTopBar.h"
#include <Ludens/UI/UIContext.h>

namespace LD {

class TopBarOption
{
public:
    static TopBarOption* create(UINode node, const char* cstr)
    {
        auto opt = heap_new<TopBarOption>(MEMORY_USAGE_UI);

        UILayoutInfo layoutI{};
        layoutI.sizeX = UISize::fit();
        layoutI.sizeY = UISize::grow();

        UIPanelWidgetInfo panelWI{};
        opt->mPanel = node.add_panel(layoutI, panelWI, opt);
        opt->mPanel.set_on_draw(&TopBarOption::on_draw);
        opt->mPanel.set_on_enter(&TopBarOption::on_enter);
        opt->mPanel.set_on_mouse_down(&TopBarOption::on_mouse_down);

        UITextWidgetInfo textWI{};
        textWI.cstr = cstr;
        textWI.fontSize = 16; // TODO:
        textWI.hoverHL = true;
        opt->mText = opt->mPanel.node().add_text({}, textWI, opt);
        opt->mText.set_on_mouse_down(&TopBarOption::on_mouse_down);
        opt->mText.set_on_enter(&TopBarOption::on_enter);

        UIContext ctx(node.get_context());

        UIWindowInfo windowI{};
        windowI.name = cstr;
        windowI.defaultMouseControls = false;

        layoutI.childGap = 4;
        layoutI.sizeX = UISize::fixed(512);
        layoutI.sizeY = UISize::fixed(512);
        opt->mWindow = ctx.add_window(layoutI, windowI, opt);
        opt->mWindow.set_on_draw(&TopBarOption::on_window_draw);
        opt->mWindow.set_on_leave(&TopBarOption::on_window_leave);
        opt->mWindow.hide();

        return opt;
    }

    static void destroy(TopBarOption* opt)
    {
        heap_delete<TopBarOption>(opt);
    }

    static void on_enter(UIWidget widget)
    {
        auto& self = *(TopBarOption*)widget.get_user();

        Rect rect = widget.get_rect();
        Vec2 pos(rect.x, rect.y + rect.h);
        self.mWindow.set_pos(pos);
    }

    static void on_mouse_down(UIWidget widget, const Vec2& pos, MouseButton btn)
    {
        auto& self = *(TopBarOption*)widget.get_user();

        self.mWindow.show();
    }

    static void on_draw(UIWidget widget, ScreenRenderComponent renderer)
    {
        auto& self = *(TopBarOption*)widget.get_user();

        Rect rect = widget.get_rect();
        //renderer.draw_rect_outline(rect, 2.0f, 0x22FF11FF);
    }

    static void on_window_draw(UIWidget widget, ScreenRenderComponent renderer)
    {
        auto& self = *(TopBarOption*)widget.get_user();

        renderer.draw_rect(widget.get_rect(), 0x22FFFFFF);
    }

    static void on_window_leave(UIWidget widget)
    {
        auto& self = *(TopBarOption*)widget.get_user();

        UIWindow window = (UIWindow)widget;
        window.hide();
    }

    void draw_overlay(ScreenRenderComponent renderer)
    {
        if (mWindow.is_hidden())
            return;

        uint32_t sw, sh;
        renderer.get_screen_extent(sw, sh);

        Rect rect = mWindow.get_rect();
        RImage blurBG = renderer.get_sampled_image();
        Rect uv;
        uv.x = rect.x / sw;
        uv.y = rect.y / sh;
        uv.w = rect.w / sw;
        uv.h = rect.h / sh;
        renderer.draw_image_uv(rect, blurBG, uv, 0xFFFFFFFF);
    }

private:
    UIPanelWidget mPanel; /// option panel
    UITextWidget mText;   /// option text on top of panel
    UIWindow mWindow;     /// option window
};

void EditorTopBar::startup(UIWindow root)
{
    mRoot = root;
    mRoot.set_on_draw(&EditorTopBar::on_draw);
    mFileOption = TopBarOption::create(mRoot.node(), "file");
    mHelpOption = TopBarOption::create(mRoot.node(), "help");
}

void EditorTopBar::cleanup()
{
    TopBarOption::destroy(mHelpOption);
    TopBarOption::destroy(mFileOption);
}

void EditorTopBar::draw_overlay(ScreenRenderComponent renderer)
{
    mFileOption->draw_overlay(renderer);
    mHelpOption->draw_overlay(renderer);
}

void EditorTopBar::on_draw(UIWidget widget, ScreenRenderComponent renderer)
{
    UIWindow window = (UIWindow)widget;

    std::vector<UIWidget> widgets;
    window.get_widgets(widgets);

    for (UIWidget widget : widgets)
    {
        widget.on_draw(renderer);
    }
}

} // namespace LD