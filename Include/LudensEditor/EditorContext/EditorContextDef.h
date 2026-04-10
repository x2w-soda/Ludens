#pragma once

#include <Ludens/System/FileSystem.h>

namespace LD {

struct EditorUpdateTick
{
    float delta;     // delta time in seconds
    Vec2 screenSize; // window size
};

struct EditorProjectEntry
{
    FS::Path schemaPath;
    std::string projectName;
};

enum EditorUIMainLayout
{
    EDITOR_UI_MAIN_LAYOUT_SCENE = 0,
    EDITOR_UI_MAIN_LAYOUT_DOCS,
};

} // namespace LD