#include <Ludens/RenderServer/ScreenLayer.h>
#include <Ludens/System/Memory.h>

namespace LD {

/// @brief Screen layer implementation.
struct ScreenLayerObj
{
    std::vector<ScreenLayerItem> drawList;
    bool isRecording = false;
};

ScreenLayer ScreenLayer::create()
{
    auto* obj = heap_new<ScreenLayerObj>(MEMORY_USAGE_RENDER);

    return ScreenLayer(obj);
}

void ScreenLayer::destroy(ScreenLayer layer)
{
    auto* obj = layer.unwrap();

    heap_delete<ScreenLayerObj>(obj);
}

void ScreenLayer::begin()
{
    mObj->isRecording = true;
    mObj->drawList.clear();
}

void ScreenLayer::end()
{
    mObj->isRecording = false;

    // TODO: Z depth radix sort
}

void ScreenLayer::add_rect(const Rect& rect, const Color& color, uint32_t zDepth)
{
    ScreenLayerItem item;
    item.image = {};
    item.rect = rect;
    item.color = color;
    item.zDepth = zDepth;
    mObj->drawList.push_back(item);
}

void ScreenLayer::add_image(const Rect& rect, RImage image, uint32_t zDepth)
{
    ScreenLayerItem item;
    item.image = image;
    item.rect = rect;
    item.color = Color(0xFFFFFFFF);
    item.zDepth = zDepth;
    mObj->drawList.push_back(item);
}

const std::vector<ScreenLayerItem>& ScreenLayer::get_draw_list() const
{
    return mObj->drawList;
}

} // namespace LD