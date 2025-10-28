#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/System/FileSystem.h>
#include <Ludens/UI/UIContext.h>
#include <LudensEditor/EditorContext/EditorContext.h>
#include <LudensEditor/EditorContext/EditorSettings.h>
#include <vector>

namespace LD {

struct EUISelectWindowRow
{
    struct EUISelectWindow* window;
    int idx;
};

struct EUISelectWindow
{
    UIWindow client;
    EditorTheme theme;
    RImage editorIconAtlas;
    const char* extensionFilter;
    const char* clientName;
    std::vector<FS::Path> directoryContents;
    std::vector<EUISelectWindowRow> rows;
    FS::Path directoryPath;
    int highlightedItemIndex = -1;
    bool isActive;
    bool isContentDirty = true;
    void (*onSelect)(const FS::Path& selected, void* user) = nullptr;
    void* user = nullptr;
};

bool eui_select_window(EUISelectWindow* window, FS::Path& selectedPath);

struct UISelectWindowInfo
{
    UIContext context;
    EditorContext editorCtx;
    FS::Path directory;
};

} // namespace LD