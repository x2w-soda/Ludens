#pragma once

#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIWidget.h>

// OVERVIEW
//
// - This is a thin immediate-mode API layer that is completely optional,
//   this layer wraps the standard UIWindow and UIWidget API for rapid
//   prototyping. While this is useful for the Editor, the final game
//   Runtime will most likely wish to avoid the overhead of this layer.
//
// - Since the UIWindow/UIWidget is a tree hierarchy, a stack-based
//   API is required to specify the tree hierarchy without ambiguity.
//

namespace LD {

/// @brief Release and free all resources allocated by immediate-mode API.
void ui_imgui_release(UIContext ctx);

/// @brief Begin immediate mode frame.
/// @param ctx The context to connect to.
void ui_frame_begin(UIContext ctx);

/// @brief End the immediate mode frame.
void ui_frame_end();

/// @brief Set layout of widget on top of stack.
void ui_top_layout(const UILayoutInfo& layoutI);

/// @brief Set user of widget on top of stack.
void ui_top_user(void* user);

/// @brief Check if widget on top of stack has UI_MOUSE_DOWN event.
/// @return True if event exists.
/// @warning Overrides existing on_mouse callback on widget.
bool ui_top_mouse_down(MouseButton& outButton);

/// @brief Check if widget on top of stack has UI_MOUSE_UP event.
/// @return True if event exists.
/// @warning Overrides existing on_mouse callback on widget.
bool ui_top_mouse_up(MouseButton& outButton);

/// @brief Check if widget on top of stack has UI_KEY_DOWN event.
/// @return True if event exists.
/// @warning Overrides existing on_key callback on widget.
bool ui_top_key_down(KeyCode& outKey);

/// @brief Check if widget on top of stack has UI_KEY_UP event.
/// @return True if event exists.
/// @warning Overrides existing on_key callback on widget.
bool ui_top_key_up(KeyCode& outKey);

/// @brief Pop the most recent non-window widget.
void ui_pop();

/// @brief Pop current window.
void ui_pop_window();

/// @brief Push UIWindow.
void ui_push_window(const char* name);

/// @brief Push existing UIWindow as client.
void ui_push_window_client(const char* name, UIWindow client);

/// @brief Sets the current window to position and fixed size.
void ui_set_window_rect(const Rect& rect);

/// @brief Check if an external UIWindow client has previously been pushed.
bool ui_has_window_client(const char* name);

/// @brief Push UITextWidget.
void ui_push_text(const char* text);

/// @brief Push UIImageWidget.
void ui_push_image(RImage image, float width, float height);

/// @brief Push UIPanelWidget.
void ui_push_panel();

/// @brief Push UIToggleWidget.
void ui_push_toggle(bool& isPressed, bool& state);

/// @brief Push UIScrollWidget.
void ui_push_scroll();

/// @brief Push UIButtonWidget.
void ui_push_button(const char* text, bool& isPressed);

/// @brief Push UISliderWidget.
void ui_push_slider(float minValue, float maxValue, float* value);

} // namespace LD