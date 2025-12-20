#include <Ludens/Header/Math/Mat3.h>
#include <Ludens/Header/Math/Transform.h>
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

void ScreenLayer::add_image(const Transform2D& transform, const Rect& rect, RImage image, uint32_t zDepth)
{
    Mat3 modelMat = transform.as_mat3();
    Vec3 tl = modelMat * Vec3(rect.get_pos(), 1.0f);
    Vec3 tr = modelMat * Vec3(rect.get_pos_tr(), 1.0f);
    Vec3 br = modelMat * Vec3(rect.get_pos_br(), 1.0f);
    Vec3 bl = modelMat * Vec3(rect.get_pos_bl(), 1.0f);

    ScreenLayerItem item;
    item.image = image;
    item.color = Color(0xFFFFFFFF);
    item.zDepth = zDepth;
    item.tl = Vec2(tl.x / tl.z, tl.y / tl.z);
    item.tr = Vec2(tr.x / tr.z, tr.y / tr.z);
    item.br = Vec2(br.x / br.z, br.y / br.z);
    item.bl = Vec2(bl.x / bl.z, bl.y / bl.z);
    mObj->drawList.push_back(item);
}

const std::vector<ScreenLayerItem>& ScreenLayer::get_draw_list() const
{
    return mObj->drawList;
}

} // namespace LD