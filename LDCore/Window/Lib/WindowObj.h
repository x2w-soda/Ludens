#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Window/Window.h>
#include <cstdint>

struct GLFWwindow;
struct GLFWcursor;

namespace LD {

/// @brief Window implementation. Currently a brief layer on top of GLFW.
class WindowObj
{
public:
    WindowObj() = delete;
    WindowObj(const WindowInfo& windowI);
    WindowObj(const Window&) = delete;
    WindowObj(Window&&) = delete;
    ~WindowObj();

    WindowObj& operator=(const WindowObj&) = delete;
    WindowObj& operator=(WindowObj&&) = delete;

    inline uint32_t width() { return mWidth; }
    inline uint32_t height() { return mHeight; }
    inline float aspect_ratio() { return (float)mWidth / (float)mHeight; }
    inline GLFWwindow* get_glfw_handle() { return mHandle; }
    inline double get_delta_time() { return mTimeDelta; }
    inline void exit() { mIsAlive = false; }

    void frame_boundary();
    void poll_events();
    void on_event(const Event* event);
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
    static void scroll_callback(GLFWwindow* handle, double xoffset, double yoffset);

    GLFWwindow* mHandle = nullptr;
    GLFWcursor* mCursors[CURSOR_TYPE_ENUM_COUNT];
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    void* mUser = nullptr;
    void (*mOnEvent)(const Event* event, void* user) = nullptr;
    bool mIsAlive = false;
    double mTimeDelta = 0.0;
    double mTimePrevFrame = 0.0;
    double mTimeThisFrame = 0.0;
};

} // namespace LD