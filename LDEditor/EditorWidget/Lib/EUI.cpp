#include <Ludens/DSA/Stack.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/UI/UIImmediate.h>
#include <Ludens/WindowRegistry/WindowRegistry.h>
#include <LudensEditor/EditorWidget/EditorWidget.h>

#include "EUI.h"

namespace LD {

static EditorContext sCtx;
static Stack<EditorTheme> sEUIThemeStack;
static WindowID sEUIWindowID;
static CursorType sEUIWindowCursor;

void push_prop_hbox()
{
    EditorTheme theme = eui_get_theme();
    UILayoutInfo layoutI = theme.make_hbox_layout();
    layoutI.sizeX = UISize::grow();
    layoutI.childGap = theme.get_child_gap_large();
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);
}

void pop_prop_hbox()
{
    ui_pop();
}

void push_prop_edit_vbox()
{
    EditorTheme theme = eui_get_theme();
    UILayoutInfo layoutI{};
    layoutI.childAxis = UI_AXIS_Y;
    layoutI.sizeX = UISize::fit();
    layoutI.sizeY = UISize::fit();
    layoutI.childGap = theme.get_child_gap_large();
    ui_push_panel(nullptr);
    ui_top_layout(layoutI);
}

void pop_prop_edit_vbox()
{
    ui_pop();
}

bool push_text_edit_f32(UITextEditData* edit, float* f32, std::string& str, bool normalized)
{
    bool commit = false;

    (void)ui_push_text_edit(edit);
    edit->set_domain(UI_TEXT_EDIT_DOMAIN_F32);
    if (!ui_text_edit_is_editing())
        edit->set_text(std::format("{:8.3f}", *f32));
    if (ui_text_edit_submitted(str))
    {
        *f32 = std::stof(str);
        if (normalized)
            *f32 = std::clamp(*f32, 0.0f, 1.0f);
        commit = true;
    }

    return commit;
}

void pop_text_edit_f32()
{
    ui_pop();
}

void eui_startup(EditorContext ctx)
{
    sCtx = ctx;
}

void eui_cleanup()
{
    LD_ASSERT(sCtx && sEUIThemeStack.empty());

    sCtx = {};
}

void eui_push_theme(EditorTheme theme)
{
    LD_ASSERT(theme);

    sEUIThemeStack.push(theme);
}

void eui_pop_theme()
{
    LD_ASSERT(!sEUIThemeStack.empty());

    sEUIThemeStack.pop();
}

void eui_begin_window(WindowID id)
{
    LD_ASSERT(id);

    sEUIWindowID = id;
    sEUIWindowCursor = CURSOR_TYPE_DEFAULT;
}

CursorType eui_get_window_cursor()
{
    LD_ASSERT(sEUIWindowID);

    return sEUIWindowCursor;
}

void eui_set_window_cursor(CursorType cursor)
{
    LD_ASSERT(sEUIWindowID);

    sEUIWindowCursor = cursor;
}

void eui_end_window()
{
    WindowRegistry::get().hint_window_cursor_shape(sEUIWindowID, sEUIWindowCursor);
    sEUIWindowID = 0;
}

EditorTheme eui_get_theme()
{
    LD_ASSERT(!sEUIThemeStack.empty());

    return sEUIThemeStack.top();
}

EditorContext eui_get_context()
{
    LD_ASSERT(sCtx);

    return sCtx;
}

} // namespace LD