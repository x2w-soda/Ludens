#pragma once
#pragma once

#include <Ludens/Header/Color.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <vector>

namespace LD {

struct ScreenLayerItem
{
    RImage image;
    Rect rect;
    Color color;
    uint32_t zDepth;
};

struct ScreenLayer : Handle<struct ScreenLayerObj>
{
    static ScreenLayer create();
    static void destroy(ScreenLayer layer);

    void begin();
    void end();

    void add_rect(const Rect& rect, const Color& color, uint32_t zDepth);
    void add_image(const Rect& rect, RImage image, uint32_t zDepth);

    const std::vector<ScreenLayerItem>& get_draw_list() const;
};

} // namespace LD