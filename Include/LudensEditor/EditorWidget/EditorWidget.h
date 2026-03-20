#pragma once

#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorSettings.h>

namespace LD {

void eui_startup(EditorContext ctx);
void eui_cleanup();
void eui_push_theme(EditorTheme theme);
void eui_pop_theme();

} // namespace LD