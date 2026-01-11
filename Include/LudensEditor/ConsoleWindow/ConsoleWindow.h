#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UIWindow.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorWindow.h>

namespace LD {

/// @brief Editor console window.
struct ConsoleWindow : Handle<struct ConsoleWindowObj>
{
    ConsoleWindow() = default;
    ConsoleWindow(const EditorWindowObj* obj) { mObj = (ConsoleWindowObj*)obj; }

    /// @brief Create console window.
    static EditorWindow create(const EditorWindowInfo& editorI);

    /// @brief Destroy console window.
    static void destroy(EditorWindow window);

    /// @brief Registers an observer to dump logs in editor console window.
    void observe_channel(const char* channelName);
};

} // namespace LD