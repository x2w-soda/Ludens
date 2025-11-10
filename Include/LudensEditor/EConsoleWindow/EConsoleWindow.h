#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/UI/UIWindow.h>
#include <Ludens/UI/UIWindowManager.h>
#include <LudensEditor/EditorContext/EditorCallback.h>
#include <LudensEditor/EditorContext/EditorContext.h>

namespace LD {

struct EConsoleWindowInfo
{
    EditorContext ctx;              /// editor context handle
    UIWindowManager wm;             /// window manager handle
    UIWMAreaID areaID;              /// designated window area
    void* user;                     /// used in callbacks
};

/// @brief Editor inspector window.
///        Displays the properties of the selected object.
struct EConsoleWindow : Handle<struct EConsoleWindowObj>
{
    /// @brief Create console window.
    static EConsoleWindow create(const EConsoleWindowInfo& windowInfo);

    /// @brief Destroy console window.
    static void destroy(EConsoleWindow window);

    /// @brief Registers an observer to dump logs in editor console window.
    void observe_channel(const char* channelName);
};

} // namespace LD