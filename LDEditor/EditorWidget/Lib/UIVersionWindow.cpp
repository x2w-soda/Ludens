#include <Ludens/Header/Version.h>
#include <LudensEditor/EditorWidget/UIVersionWindow.h>
#include <format>

namespace LD {

struct UIVersionWindowObj
{
    UIContext uiCtx;
    EditorTheme theme;
    UIWindow root;
    UITextWidget versionTextW;
    UITextWidget buildTextW;
};

UIVersionWindow UIVersionWindow::create(const UIVersionWindowInfo& info)
{
    UIVersionWindowObj* obj = heap_new<UIVersionWindowObj>(MEMORY_USAGE_UI);
    obj->uiCtx = info.context;
    obj->theme = info.theme;

    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    UIWindowInfo windowI{};
    windowI.defaultMouseControls = false;
    windowI.drawWithScissor = false;
    windowI.name = "Version";
    obj->root = obj->uiCtx.add_window(layoutI, windowI, obj);

    float fontSize = obj->theme.get_font_size();
    std::string version = std::format("Version {}.{}.{}", LD_VERSION_MAJOR, LD_VERSION_MINOR, LD_VERSION_PATCH);

#ifdef NDEBUG
    std::string build("Release Build");
#else
    std::string build("Debug Build");
#endif

    UITextWidgetInfo textWI{};
    textWI.fontSize = fontSize;
    textWI.hoverHL = false;
    textWI.cstr = version.c_str();
    obj->versionTextW = obj->root.node().add_text(layoutI, textWI, obj);

    textWI.cstr = build.c_str();
    obj->buildTextW = obj->root.node().add_text(layoutI, textWI, obj);

    obj->root.layout();

    return UIVersionWindow(obj);
}

void UIVersionWindow::destroy(UIVersionWindow window)
{
    UIVersionWindowObj* obj = window.unwrap();

    obj->uiCtx.remove_window(obj->root);

    heap_delete<UIVersionWindowObj>(obj);
}

UIWindow UIVersionWindow::get_handle()
{
    return mObj->root;
}

} // namespace LD