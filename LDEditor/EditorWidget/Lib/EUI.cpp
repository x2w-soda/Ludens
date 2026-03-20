#include <Ludens/DSA/Stack.h>
#include <Ludens/Header/Assert.h>
#include <LudensEditor/EditorWidget/EditorWidget.h>

#include "EUI.h"

namespace LD {

static EditorContext sCtx;
static Stack<EditorTheme> sEUIThemeStack;

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