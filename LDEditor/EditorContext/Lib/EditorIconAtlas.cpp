#include <Ludens/DataRegistry/DataRegistry.h>
#include <LudensEditor/EditorContext/EditorIconAtlas.h>

#define ICON_SIZE 48
#define ATLAS_WIDTH 512.0f
#define ATLAS_HEIGHT 512.0f

namespace LD {

struct
{
    EditorIcon icon;
    Vec2 pos;
} sIconTable[] = {
    {EDITOR_ICON_ADD_FOLDER, Vec2(0, 0)},
    {EDITOR_ICON_ANIMATED_SPRITE2D_COMPONENT, Vec2(48, 0)},
    {EDITOR_ICON_ARROW_UP, Vec2(96, 0)},
    {EDITOR_ICON_AUDIO_SOURCE_COMPONENT, Vec2(144, 0)},
    {EDITOR_ICON_CAMERA_COMPONENT, Vec2(192, 0)},
    {EDITOR_ICON_CLOSE, Vec2(240, 0)},
    {EDITOR_ICON_CONSOLE_WINDOW, Vec2(288, 0)},
    {EDITOR_ICON_FILE, Vec2(336, 0)},
    {EDITOR_ICON_FOLDER, Vec2(384, 0)},
    {EDITOR_ICON_INSPECTOR_WINDOW, Vec2(432, 0)},
    {EDITOR_ICON_MESH_COMPONENT, Vec2(0, 48)},
    {EDITOR_ICON_OFFSET, Vec2(48, 48)},
    {EDITOR_ICON_OUTLINER_WINDOW, Vec2(96, 48)},
    {EDITOR_ICON_PAUSE, Vec2(144, 48)},
    {EDITOR_ICON_PLAY, Vec2(192, 48)},
    {EDITOR_ICON_REFRESH, Vec2(240, 48)},
    {EDITOR_ICON_ROTATE, Vec2(288, 48)},
    {EDITOR_ICON_SCALE, Vec2(336, 48)},
    {EDITOR_ICON_SCREEN_UI_COMPONENT, Vec2(384, 48)},
    {EDITOR_ICON_SCRIPT, Vec2(432, 48)},
    {EDITOR_ICON_SKELETAL_MESH_COMPONENT, Vec2(0, 96)},
    {EDITOR_ICON_SPRITE2D_COMPONENT, Vec2(48, 96)},
    {EDITOR_ICON_TILEMAP_COMPONENT, Vec2(96, 96)},
    {EDITOR_ICON_TRANSFORM_COMPONENT, Vec2(144, 96)},
    {EDITOR_ICON_VIEWPORT_WINDOW, Vec2(192, 96)},
};

static_assert(sizeof(sIconTable) / sizeof(*sIconTable) == (int)EDITOR_ICON_ENUM_LAST);

const EditorIcon sComponentIconTable[] = {
    EDITOR_ICON_ENUM_LAST,
    EDITOR_ICON_AUDIO_SOURCE_COMPONENT,
    EDITOR_ICON_TRANSFORM_COMPONENT,
    EDITOR_ICON_CAMERA_COMPONENT,
    EDITOR_ICON_MESH_COMPONENT,
    EDITOR_ICON_SPRITE2D_COMPONENT,
    EDITOR_ICON_SCREEN_UI_COMPONENT,
};

static_assert(sizeof(sComponentIconTable) / sizeof(*sComponentIconTable) == (int)COMPONENT_TYPE_ENUM_COUNT);

EditorIcon EditorIconAtlas::get_component_icon(const ComponentType& type)
{
    LD_ASSERT(0 <= (int)type && (int)type < (int)COMPONENT_TYPE_ENUM_COUNT);

    return sComponentIconTable[(int)type];
}

Rect EditorIconAtlas::get_component_icon_rect(const ComponentType& type)
{
    EditorIcon icon = get_component_icon(type);

    if (icon == EDITOR_ICON_ENUM_LAST)
        return {};

    return get_icon_rect(icon);
}

Rect EditorIconAtlas::get_icon_rect(EditorIcon icon)
{
    LD_ASSERT(0 <= (int)icon && (int)icon < (int)EDITOR_ICON_ENUM_LAST);

    Vec2 pos = sIconTable[(int)icon].pos;

    return Rect(pos.x, pos.y, ICON_SIZE, ICON_SIZE);
}

Rect EditorIconAtlas::get_icon_rect_uv(EditorIcon icon)
{
    Rect rect = get_icon_rect(icon);

    return Rect(rect.x / ATLAS_WIDTH, rect.y / ATLAS_HEIGHT, rect.w / ATLAS_WIDTH, rect.h / ATLAS_HEIGHT);
}

} // namespace LD