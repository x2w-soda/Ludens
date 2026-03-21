#pragma once

#include <Ludens/UI/UIContext.h>
#include <Ludens/UI/UIFont.h>
#include <Ludens/UI/UIWidget.h>
#include <Ludens/UI/Widgets.h>

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
struct WindowEvent;

typedef void (*IMDrawCallback)(UIWidget widget, ScreenRenderComponent renderer, void* user);

void ui_imgui_startup(UIFont font);

/// @brief Release and free all resources allocated by immediate-mode API.
void ui_imgui_cleanup();

bool ui_context_input(const char* ctxName, const WindowEvent* event);
void ui_context_render(const char* ctxName, ScreenRenderComponent renderer);

/// @brief Begin immediate mode context.
/// @param ctx Unique context name.
/// @param screenExtent The screen size for this frame.
/// @param delta Delta time for this frame.
void ui_context_begin(const char* ctxName, const Vec2& screenExtent);

/// @brief End the immediate mode context.
void ui_context_end(float delta);

/// @brief Begin a UILayer scope.
/// @param layerName Unique layer name within context.
void ui_layer_begin(const char* layerName);

/// @brief End the current layer scope.
void ui_layer_end();

/// @brief Begin a UIWorkspace scope.
/// @param workspaceName  Unique workspace name within layer.
void ui_workspace_begin(const char* workspaceName, const Rect& workspaceArea);

/// @brief End the current workspace scope.
void ui_workspace_end();

/// @brief Set layout of widget on top of stack.
void ui_top_layout(const UILayoutInfo& layoutI);

/// @brief Set size of widget on top of stack.
void ui_top_layout_size(const UISize& sizeX, const UISize& sizeY);

/// @brief Set child axis of widget on top of stack.
void ui_top_layout_child_axis(UIAxis childAxis);

/// @brief Set child padding of widget on top of stack.
void ui_top_layout_child_padding(const UIPadding& pad);

/// @brief Set child gap of widget on top of stack.
void ui_top_layout_child_gap(float childGap);

/// @brief Set user of widget on top of stack.
void ui_top_user(void* user);

/// @brief Get rect of widget on top of stack.
void ui_top_get_rect(Rect& outRect);

/// @brief Get relative mouse position of widget on top of stack,
///        if the widget rect contains mouse position.
bool ui_top_get_mouse_pos(Vec2& outMousePos);

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

/// @brief Push window to decorate.
/// @param windowName Unique window name in workspace
void ui_push_window(const char* windowName);

void ui_window_set_color(Color color);

/// @brief Sets the current window to position and fixed size.
void ui_window_set_rect(const Rect& rect);

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
UITextStorage* ui_push_text(UITextStorage* storage, const char* text);
void ui_text_style(Color color, UIFont font);

/// @brief Push UITextEditWidget.
UITextEditStorage* ui_push_text_edit(UITextEditStorage* storage);
void ui_text_edit_domain(UITextEditDomain domain);
void ui_text_edit_set_text(View text);
inline void ui_text_edit_set_text(const std::string& text) { ui_text_edit_set_text(View(text.data(), text.size())); }
bool ui_text_edit_changed(std::string& text);
bool ui_text_edit_submitted(std::string& text);

/// @brief Push UIImageWidget.
UIImageStorage* ui_push_image(UIImageStorage* storage, float width, float height);

/// @brief Push UIPanelWidget.
UIPanelStorage* ui_push_panel(UIPanelStorage* storage);

/// @brief Push UIToggleWidget.
UIToggleStorage* ui_push_toggle(UIToggleStorage* storage);
bool ui_toggle_is_pressed();

/// @brief Push UIScrollWidget.
UIScrollStorage* ui_push_scroll(UIScrollStorage* storage);

/// @brief Push UIButtonWidget.
UIButtonStorage* ui_push_button(UIButtonStorage* storage, const char* text);
bool ui_button_is_pressed();

/// @brief Push UISliderWidget.
UISliderStorage* ui_push_slider(UISliderStorage* storage, float* value);

} // namespace LD