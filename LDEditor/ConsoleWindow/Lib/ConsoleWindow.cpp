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
    LogLevel level;

    std::string format() const
    {
        std::chrono::duration<double> duration(sessionTime);
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);

        return std::format("[{:%T}][{}] {}", ms, channel, message);
    }
};

static Vector<ConsoleEntry> sHistory;

static void console_log_writeback(LogLevel level, const std::string& ch, const std::string& msg, void* user)
{
    ConsoleEntry entry{};
    entry.sessionTime = WindowRegistry::get().get_time();
    entry.channel = ch;
    entry.message = msg;
    entry.level = level;

    sHistory.push_back(entry);
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

    FontAtlas fontAtlas;
    RImage fontImage;
    ctx.get_mono_font(fontAtlas, fontImage);

    for (const ConsoleEntry& entry : sHistory)
    {
        std::string text = entry.format();

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

        ui_push_text(text.c_str());
        ui_text_style(color, fontAtlas, fontImage);
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