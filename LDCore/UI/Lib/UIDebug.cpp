#include "UIObj.h"

#include "UIDebug.h"

#include <format>

#define INDENT_SPACES 2

namespace LD {

void UIDebug::print_context(UIContextObj* ctx)
{
    ss.clear();
    printIndent = 0;

    for (UILayerObj* layer : ctx->layers)
        print_layer(layer);
}

void UIDebug::print_layer(UILayerObj* layer)
{
    ss << std::string(printIndent, ' ') << "UILayer: " << layer->name << std::endl;
    printIndent += INDENT_SPACES;

    for (UIWorkspaceObj* space : layer->workspaces)
        print_workspace(space);

    printIndent -= INDENT_SPACES;
}

void UIDebug::print_workspace(UIWorkspaceObj* workspace)
{
    ss << std::string(printIndent, ' ') << "UIWorkspace" << std::endl;
    printIndent += INDENT_SPACES;

    for (UIWindowObj* window : workspace->nodeWindows)
        print_window(window);

    for (UIWindowObj* window : workspace->floatWindows)
        print_window(window);
    printIndent -= INDENT_SPACES;
}

void UIDebug::print_window(UIWindowObj* window)
{
    ss << std::string(printIndent, ' ') << "UIWindow: " << window->name << std::endl;
    printIndent += INDENT_SPACES;

    if (printWidgetTree)
    {
        for (UIWidgetObj* child = window->child; child; child = child->next)
            print_widget_tree(child);
    }

    printIndent -= INDENT_SPACES;
}

void UIDebug::print_widget_tree(UIWidgetObj* widget)
{
    const char* cstr = get_ui_widget_type_cstr(widget->type);

    ss << std::string(printIndent, ' ') << std::format("{}: {}", cstr, widget->name) << std::endl;
    printIndent += INDENT_SPACES;

    for (UIWidgetObj* child = widget->child; child; child = child->next)
    {
        print_widget_tree(child);
    }

    printIndent -= INDENT_SPACES;
}

} // namespace LD