#pragma once

#include <Ludens/WindowRegistry/WindowRegistryDef.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

void eui_startup(EditorContext ctx);
void eui_cleanup();
void eui_push_theme(EditorTheme theme);
void eui_pop_theme();
void eui_begin_window(WindowID id);
CursorType eui_get_window_cursor();
void eui_set_window_cursor(CursorType type);
void eui_end_window();

} // namespace LD