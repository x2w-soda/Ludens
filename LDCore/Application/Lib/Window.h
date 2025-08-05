#pragma once

#include <Ludens/Application/Application.h>
#include <Ludens/Header/Color.h>
#include <cstdint>

struct GLFWwindow;
struct GLFWcursor;

namespace LD {

/// @brief Application Window implementation. Essentially a wrapper around GLFW
class Window
{
public:
    Window();
    Window(const Window&) = delete;
    ~Window() = default;

    Window& operator=(const Window&) = delete;

    void startup(const ApplicationInfo& appI);
    void cleanup();

    inline uint32_t width() { return mWidth; }
    inline uint32_t height() { return mHeight; }
    inline float aspect_ratio() { return (float)mWidth / (float)mHeight; }

    GLFWwindow* get_glfw_handle();
    void poll_events();
    void get_cursor_pos(double& xpos, double& ypos);
    void set_cursor_mode_normal();
    void set_cursor_mode_disabled();
    double get_time();
    bool is_open();

    void hint_border_color(Color color);
    void hint_title_bar_color(Color color);
    void hint_title_bar_text_color(Color color);
    void hint_title_bar_text(const char* cstr);
    void hint_cursor_shape(CursorType cursor);

private:
    static void size_callback(GLFWwindow* window, int width, int height);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);

    GLFWwindow* mHandle;
    GLFWcursor* mCursors[CURSOR_TYPE_ENUM_COUNT];
    uint32_t mWidth;
    uint32_t mHeight;
};

} // namespace LD