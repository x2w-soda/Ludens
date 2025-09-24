#include <LudensEditor/EditorContext/EditorIconAtlas.h>

#define ICON_SIZE 48
#define ATLAS_WIDTH 512.0f
#define ATLAS_HEIGHT 512.0f

namespace LD {

// clang-format off
struct
{
    EditorIcon icon;
    Vec2 pos;
} sIconTable[] = {
    { EditorIcon::Folder,      Vec2(288, 96) },
    { EditorIcon::Description, Vec2(240, 96) },
    { EditorIcon::Close,       Vec2(240, 144) },
    { EditorIcon::Refresh,     Vec2(384, 144) },
    { EditorIcon::Remove,      Vec2(288, 0) },
};
// clang-format on

static_assert(sizeof(sIconTable) / sizeof(*sIconTable) == (int)EditorIcon::ENUM_COUNT);

Rect EditorIconAtlas::get_icon_rect(EditorIcon icon)
{
    Vec2 pos = sIconTable[(int)icon].pos;

    return Rect(pos.x, pos.y, ICON_SIZE, ICON_SIZE);
}

Rect EditorIconAtlas::get_icon_rect_uv(EditorIcon icon)
{
    Rect rect = get_icon_rect(icon);

    return Rect(rect.x / ATLAS_WIDTH, rect.y / ATLAS_HEIGHT, rect.w / ATLAS_WIDTH, rect.h / ATLAS_HEIGHT);
}

} // namespace LD