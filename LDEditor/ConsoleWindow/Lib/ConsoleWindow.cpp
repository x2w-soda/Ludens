#include <Ludens/DSA/Vector.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIImmediate.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>
#include <LudensEditor/ConsoleWindow/ConsoleWindow.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

#include <chrono>
#include <format>
#include <string>

namespace LD {

struct ConsoleEntry
{
    double sessionTime;
    std::string channel;
    std::string message;
    std::string formatted;
    LogLevel level;
};

static Vector<ConsoleEntry> sHistory;

static void console_log_writeback(LogLevel level, const std::string& ch, const std::string& msg, void* user)
{
    LD_PROFILE_SCOPE;

    ConsoleEntry entry{};
    entry.sessionTime = WindowRegistry::get().get_time();
    entry.channel = ch;
    entry.message = msg;
    entry.level = level;

    std::chrono::duration<double> duration(entry.sessionTime);
    std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    entry.formatted = std::format("[{:%T}][{}] {}", ms, entry.channel, entry.message);

    sHistory.push_back(entry);
}

/// @brief Editor console window implementation.
struct ConsoleWindowObj : EditorWindowObj
{
    ConsoleWindowObj(const EditorWindowInfo& info)
        : EditorWindowObj(info) {}

    virtual EditorWindowType get_type() override { return EDITOR_WINDOW_CONSOLE; }
    virtual void on_imgui(float delta) override;
};

void ConsoleWindowObj::on_imgui(float delta)
{
    LD_PROFILE_SCOPE;

    EditorTheme edTheme = mCtx.get_theme();
    UITheme uiTheme = edTheme.get_ui_theme();
    float pad = edTheme.get_child_pad();

    ui_workspace_begin();
    ui_push_window(ui_workspace_name());

    UIScrollStorage* scrollS = ui_push_scroll(nullptr);
    scrollS->bgColor = uiTheme.get_surface_color();
    
    UILayoutInfo layoutI = edTheme.make_vbox_layout();
    layoutI.sizeX = UISize::grow();
    layoutI.sizeY = UISize::grow();
    ui_top_layout(layoutI);

    UIFont font = mCtx.get_font_mono();

    for (const ConsoleEntry& entry : sHistory)
    {
        Color color;

        switch (entry.level)
        {
        case LOG_LEVEL_WARN:
        case LOG_LEVEL_ERROR:
            edTheme.get_error_color(color);
            break;
        case LOG_LEVEL_DEBUG:
        case LOG_LEVEL_INFO:
        default:
            color = uiTheme.get_on_surface_color();
            break;
        }

        ui_push_text(nullptr, entry.formatted.c_str());
        ui_text_style(color, font);
        ui_pop();
    }

    ui_pop();
    ui_pop_window();
    ui_workspace_end();
}

EditorWindow ConsoleWindow::create(const EditorWindowInfo& windowI)
{
    auto* obj = heap_new<ConsoleWindowObj>(MEMORY_USAGE_UI, windowI);

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