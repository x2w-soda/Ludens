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

struct KeyValue;
struct MouseValue;

typedef void (*IMDrawCallback)(UIWidget widget, ScreenRenderComponent renderer, void* user);

/// @brief Release and free all resources allocated by immediate-mode API.
void ui_imgui_release(UIContext ctx);

/// @brief Begin immediate mode frame.
/// @param ctx The context to connect to.
/// @param screenExtent The screen size for this frame.
void ui_frame_begin(UIContext ctx, const Vec2& screenExtent);

/// @brief End the immediate mode frame.
void ui_frame_end();

/// @brief Set layout of widget on top of stack.
void ui_top_layout(const UILayoutInfo& layoutI);

/// @brief Set size of widget on top of stack.
void ui_top_layout_size(const UISize& sizeX, const UISize& sizeY);

/// @brief Set child axis of widget on top of stack.
void ui_top_layout_child_axis(UIAxis childAxis);

/// @brief Set child padding of widget on top of stack.
void ui_top_layout_child_padding(const UIPadding& pad);

/// @brief Set child gap of widget on top of stack.
void ui_top_layout_child_gap(float gap);

/// @brief Set user of widget on top of stack.
void ui_top_user(void* user);

/// @brief Get rect of widget on top of stack.
void ui_top_rect(Rect& outRect);

/// @brief Set draw callback of widget on top of stack.
void ui_top_draw(const IMDrawCallback& imDrawCallback);

/// @brief Check if widget on top of stack has been dragged.
bool ui_top_drag(MouseButton& dragBtn, Vec2& dragPos, bool& dragBegin);

/// @brief Returns true while the widget on top of stack is dragged and not released.
bool ui_top_is_dragged();

/// @brief Check if widget on top of stack has mouse enter or mouse leave events.
bool ui_top_hover(UIEventType& hover);

/// @brief Returns true while the widget on top of stack is under mouse cursor.
bool ui_top_is_hovered();

/// @brief Check if widget on top of stack has scroll event.
bool ui_top_scroll(Vec2& scroll);

/// @brief Check if widget on top of stack has UI_MOUSE_DOWN event.
bool ui_top_mouse_down(MouseValue& outMouse, Vec2& outMousePos);

/// @brief Check if widget on top of stack has UI_MOUSE_UP event.
bool ui_top_mouse_up(MouseValue& outMouse, Vec2& outMousePos);

/// @brief Check if widget on top of stack has UI_KEY_DOWN event.
bool ui_top_key_down(KeyValue& outKey);

/// @brief Check if widget on top of stack has UI_KEY_UP event.
bool ui_top_key_up(KeyValue& outKey);

/// @brief Pop the most recent non-window widget.
void ui_pop();

/// @brief Pop current window.
void ui_pop_window();

/// @brief Push existing UIWindow as client.
void ui_push_window(UIWindow client);

void ui_window_set_color(Color color);

/// @brief Sets the current window to position and fixed size.
void ui_window_set_rect(const Rect& rect);

/// @brief Check if an external UIWindow client has previously been pushed.
bool ui_window_has_client(const char* name);

/// @brief Request the global popup window.
/// @param popupName Unique name
/// @param position Moves popup window to position.
void ui_request_popup_window(const char* popupName, const Vec2& position);

/// @brief Clear the global popup window.
void ui_clear_popup_window();

/// @brief Try push the global popup window, only succeeds if true is returned,
///        in which case the user must later call "ui_pop_window".
/// @param popupName Unique name of the popup window.
bool ui_push_popup_window(const char* popupName);

/// @brief Push UITextWidget.
void ui_push_text(const char* text);
void ui_text_style(Color color, FontAtlas fontAtlas, RImage fontImage);

/// @brief Push UITextEditWidget.
void ui_push_text_edit(UITextEditDomain domain = UI_TEXT_EDIT_DOMAIN_STRING);
void ui_text_edit_set_text(View text);
inline void ui_text_edit_set_text(const std::string& text) { ui_text_edit_set_text(View(text.data(), text.size())); }
bool ui_text_edit_changed(std::string& text);
bool ui_text_edit_submitted(std::string& text);

/// @brief Push UIImageWidget.
void ui_push_image(RImage image, float width, float height, Color tint = 0xFFFFFFFF, const Rect* portion = nullptr);

/// @brief Push UIPanelWidget.
void ui_push_panel();
void ui_panel_color(Color color);

/// @brief Push UIToggleWidget.
void ui_push_toggle(bool& isPressed, bool& state);

/// @brief Push UIScrollWidget.
void ui_push_scroll(Color bgColor);

/// @brief Push UIButtonWidget.
void ui_push_button(const char* text, bool& isPressed);

/// @brief Push UISliderWidget.
void ui_push_slider(float minValue, float maxValue, float* value);

} // namespace LD