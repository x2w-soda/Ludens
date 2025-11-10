#include <Ludens/Log/Log.h>
#include <Ludens/System/Memory.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EConsoleWindow/EConsoleWindow.h>
#include <LudensEditor/EditorContext/EditorWindowObj.h>

namespace LD {

static std::vector<std::string> sHistory;

static void console_log_writeback(LogLevel level, const std::string& msg)
{
    sHistory.push_back(msg);
}

/// @brief Editor console window implementation.
struct EConsoleWindowObj : EditorWindowObj
{
    virtual ~EConsoleWindowObj() = default;

    virtual void on_imgui() override;
};

void EConsoleWindowObj::on_imgui()
{
    EditorTheme edTheme = editorCtx.get_theme();
    UITheme uiTheme = edTheme.get_ui_theme();
    Color color = uiTheme.get_surface_color();
    float pad = edTheme.get_padding();

    ui_push_window("EConsoleWindow", root);
    ui_push_scroll(color);
    ui_top_layout_child_padding({.left = pad, .right = pad});

    for (const std::string& line : sHistory)
    {
        ui_push_text(line.c_str());
        ui_pop();
    }

    ui_pop();
    ui_pop_window();
}

EConsoleWindow EConsoleWindow::create(const EConsoleWindowInfo& windowI)
{
    UIWindowManager wm = windowI.wm;
    auto* obj = heap_new<EConsoleWindowObj>(MEMORY_USAGE_UI);

    obj->root = wm.get_area_window(windowI.areaID);
    obj->root.set_user(obj);
    obj->editorCtx = windowI.ctx;

    wm.set_window_title(windowI.areaID, "Console");
    UIWindow consoleWindow = wm.get_area_window(windowI.areaID);
    consoleWindow.set_user(obj);

    return EConsoleWindow(obj);
}

void EConsoleWindow::destroy(EConsoleWindow window)
{
    auto* obj = window.unwrap();

    heap_delete<EConsoleWindowObj>(obj);
}

void EConsoleWindow::observe_channel(const char* channelName)
{
    Log log(channelName);
    log.add_observer(&console_log_writeback);
}

} // namespace LD