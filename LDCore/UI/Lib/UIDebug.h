#pragma once

#include <sstream>
#include <string>

struct UIContextObj;
struct UILayerObj;
struct UIWorkspaceObj;
struct UIWindowObj;
struct UIWidgetObj;

namespace LD {

struct UIDebug
{
    std::stringstream ss{};
    int printIndent = 0;
    bool printWidgetTree = false;

    void print_context(UIContextObj* ctx);
    void print_layer(UILayerObj* layer);
    void print_workspace(UIWorkspaceObj* workspace);
    void print_window(UIWindowObj* window);
    void print_widget_tree(UIWidgetObj* widget);
};

} // namespace LD