#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Header/KeyCode.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>
#include <cstdint>

struct GLFWwindow;
struct GLFWcursor;

namespace LD {

/// @brief Window implementation, corresponds to a GLFWwindow.
class WindowObj
{
public:
    WindowObj() = delete;
    WindowObj(const WindowInfo& windowI, WindowID ID, WindowID parentID);
    WindowObj(const WindowObj&) = delete;
    WindowObj(WindowObj&&) = delete;
    ~WindowObj();

    WindowObj& operator=(const WindowObj&) = delete;
    WindowObj& operator=(WindowObj&&) = delete;

    inline uint32_t width() const { return mWidth; }
    inline uint32_t height() const { return mHeight; }
    inline Vec2 extent() const { return Vec2((float)mWidth, (float)mHeight); }
    inline float aspect_ratio() const { return mHeight == 0.0f ? 0.0f : (float)mWidth / (float)mHeight; }
    inline GLFWwindow* get_glfw_handle() { return mHandle; }
    inline void close() { mIsAlive = false; }
    inline bool is_alive() const { return mIsAlive; }
    inline WindowID get_id() const { return mID; }

    void frame_boundary();

    void on_event(const Event* event);

public: // decorators and hints
    void set_cursor_mode_normal();
    void set_cursor_mode_disabled();

    void hint_icon(int iconCount, Bitmap* icons);
    void hint_title_bar_text(const char* cstr);
    void hint_border_color(Color color);
    void hint_title_bar_color(Color color);
    void hint_title_bar_text_color(Color color);
    void hint_cursor_shape(CursorType cursor);

public: // key and mouse states
    bool get_key(KeyCode key);
    bool get_key_up(KeyCode key);
    bool get_key_down(KeyCode key);

    bool get_mouse(MouseButton button);
    bool get_mouse_up(MouseButton button);
    bool get_mouse_down(MouseButton button);

    void get_mouse_position(float& x, float& y);
    bool get_mouse_motion(float& dx, float& dy);

private:
    static void size_callback(GLFWwindow* window, int width, int height);
    static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow* handle, double xoffset, double yoffset);

private:
    WindowID mID = 0;
    WindowID mParentID = 0;
    uint32_t mWidth = 0;
    uint32_t mHeight = 0;
    GLFWwindow* mHandle = nullptr;
    void* mUser = nullptr;
    void (*mOnEvent)(const Event* event, void* user) = nullptr;
    bool mIsAlive = false;
    uint8_t mKeyState[KEY_CODE_ENUM_LAST];
    uint8_t mMouseState[MOUSE_BUTTON_ENUM_LAST];
    float mMouseCursorDeltaX;
    float mMouseCursorDeltaY;
    float mMouseCursorX;
    float mMouseCursorY;
};

} // namespace LD