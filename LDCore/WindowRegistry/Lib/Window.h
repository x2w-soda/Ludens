#pragma once

#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/IDCounter.h>
#include <Ludens/DSA/Observer.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Event/Event.h>
#include <Ludens/Header/Color.h>
#include <Ludens/Header/KeyCode.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>
#include <cstdint>

struct GLFWwindow;
struct GLFWcursor;

namespace LD {

class WindowObj;

/// @brief Window registry implementation.
class WindowRegistryObj
{
public:
    WindowRegistryObj();
    WindowRegistryObj(const WindowRegistryObj&) = delete;
    WindowRegistryObj(WindowRegistryObj&&) = delete;
    ~WindowRegistryObj();

    WindowRegistryObj& operator=(const WindowRegistryObj&) = delete;
    WindowRegistryObj& operator=(WindowRegistryObj&&) = delete;

    WindowObj* create_window(const WindowInfo& windowI, WindowID parentID);
    void destroy_window(WindowID id);
    void destroy_window_subtree(WindowID id);

    void frame_boundary();

    inline WindowID get_root_id() const { return mRootID; }
    inline double get_delta_time() const { return mTimeDelta; }

    inline WindowObj* get_window(WindowID id) const
    {
        auto it = mWindows.find(id);
        return it == mWindows.end() ? nullptr : it->second;
    }

    void hint_window_cursor_shape(WindowID id, CursorType cursor);
    void add_observer(const WindowEventFn fn, void* user);
    void remove_observer(WindowEventFn fn, void* user);
    void notify_observers(const WindowEvent* event);

private:
    HashMap<WindowID, WindowObj*> mWindows;
    IDCounter<WindowID> mIDCounter;
    ObserverList<const WindowEvent*> mObservers;
    WindowID mRootID = 0;
    GLFWcursor* mCursors[CURSOR_TYPE_ENUM_COUNT];
    double mTimeDelta = 0.0;
    double mTimePrevFrame = 0.0;
    double mTimeThisFrame = 0.0;
};

/// @brief Window implementation, corresponds to a GLFWwindow.
class WindowObj
{
public:
    WindowObj() = delete;
    WindowObj(const WindowInfo& windowI, WindowRegistryObj* reg, WindowID ID, WindowObj* parent);
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
    inline WindowID get_parent_id() const { return mParentID; }
    inline const Vector<WindowID>& get_children_id() const { return mChildrenID; }
    inline void erase_child_id(WindowID id) { std::erase(mChildrenID, id); }

    void frame_boundary();

    void on_event(const WindowEvent* event);

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
    Vector<WindowID> mChildrenID;
    GLFWwindow* mHandle = nullptr;
    WindowRegistryObj* mRegistry = nullptr;
    WindowEventFn mOnEvent = nullptr;
    void* mUser = nullptr;
    bool mIsAlive = false;
    uint8_t mKeyState[KEY_CODE_ENUM_LAST];
    uint8_t mMouseState[MOUSE_BUTTON_ENUM_LAST];
    float mMouseCursorDeltaX;
    float mMouseCursorDeltaY;
    float mMouseCursorX;
    float mMouseCursorY;
};

} // namespace LD