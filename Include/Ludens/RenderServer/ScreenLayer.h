#pragma once
#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Color.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/RenderBackend/RBackend.h>

namespace LD {

struct Transform2D;

struct ScreenLayerItem
{
    uint32_t zDepth;
    Color color;
    RImage image;
    Vec2 tl, tr, bl, br;
};

struct ScreenLayer : Handle<struct ScreenLayerObj>
{
    static ScreenLayer create();
    static void destroy(ScreenLayer layer);

    void begin();
    void end();

    void add_image(const Transform2D& transform, const Rect& rect, RImage image, uint32_t zDepth);

    const Vector<ScreenLayerItem>& get_draw_list() const;
};

} // namespace LD