#include <Ludens/DSA/Vector.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/ConsoleWindow/ConsoleWindow.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

#include <string>

namespace LD {

static Vector<std::string> sHistory;

static void console_log_writeback(LogLevel level, const std::string& msg)
{
    sHistory.push_back(msg);
}

/// @brief Editor console window implementation.
struct ConsoleWindowObj : EditorWindowObj
{
    EditorContext ctx;
    UIWorkspace space;
    UIWindow root;

    virtual EditorWindowType get_type() override { return EDITOR_WINDOW_CONSOLE; }
    virtual void on_imgui(float delta) override;
};

void ConsoleWindowObj::on_imgui(float delta)
{
    LD_PROFILE_SCOPE;

    EditorTheme edTheme = ctx.get_theme();
    UITheme uiTheme = edTheme.get_ui_theme();
    Color color = uiTheme.get_surface_color();
    float pad = edTheme.get_padding();

    ui_push_window(root);
    ui_push_scroll(color);

    UILayoutInfo layoutI = ctx.make_vbox_layout();
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::grow();
    ui_top_layout(layoutI);

    for (const std::string& line : sHistory)
    {
        ui_push_text(line.c_str());
        ui_pop();
    }

    ui_pop();
    ui_pop_window();
}

EditorWindow ConsoleWindow::create(const EditorWindowInfo& windowI)
{
    auto* obj = heap_new<ConsoleWindowObj>(MEMORY_USAGE_UI);
    obj->ctx = windowI.ctx;
    obj->space = windowI.space;
    obj->root = obj->space.create_window(obj->space.get_root_id(), {}, {}, nullptr);
    obj->root.show();

    return EditorWindow(obj);
}

void ConsoleWindow::destroy(EditorWindow window)
{
    auto* obj = static_cast<ConsoleWindowObj*>(window.unwrap());

    heap_delete<ConsoleWindowObj>(obj);
}

void ConsoleWindow::observe_channel(const char* channelName)
{
    Log log(channelName);
    log.add_observer(&console_log_writeback);
}

} // namespace LD