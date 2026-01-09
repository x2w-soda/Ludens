#include <Ludens/Header/Math/Mat3.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderServer/ScreenLayer.h>

#include <cstdint>
#include <cstring>

namespace LD {

/// @brief Screen layer implementation.
struct ScreenLayerObj
{
    Vector<ScreenLayerItem> drawList;
    bool isRecording = false;

    void sort_items();
};

/// @brief Linear time radix sort by u32 depth.
void ScreenLayerObj::sort_items()
{
    LD_PROFILE_SCOPE;

    constexpr uint32_t R = 256;
    constexpr uint32_t mask = R - 1;
    const uint32_t N = (uint32_t)drawList.size();

    Vector<ScreenLayerItem> tmpItems(N);
    ScreenLayerItem* src = drawList.data();
    ScreenLayerItem* dst = tmpItems.data();

    for (uint32_t pass = 0; pass < 4; pass++)
    {
        uint32_t bitShift = pass * 8;
        uint32_t hist[R];
        memset(hist, 0, sizeof(hist));

        for (uint32_t i = 0; i < N; i++)
        {
            const uint32_t byte = (src[i].zDepth >> bitShift) & mask;
            hist[byte]++;
        }

        // self-exclusive prefix sum
        uint32_t sum = 0;

        for (uint32_t i = 0; i < R; i++)
        {
            uint32_t tmp = hist[i];
            hist[i] = sum;
            sum += tmp;
        }

        for (uint32_t i = 0; i < N; i++)
        {
            const uint32_t byte = (src[i].zDepth >> bitShift) & mask;
            dst[hist[byte]++] = src[i];
        }

        std::swap(src, dst);
    }

    // just sanity checking the parity
    LD_ASSERT(src == drawList.data());
}

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
    mObj->sort_items();
}

void ScreenLayer::add_image(const Transform2D& transform, const Rect& rect, RImage image, uint32_t zDepth)
{
    LD_ASSERT(mObj->isRecording);

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

const Vector<ScreenLayerItem>& ScreenLayer::get_draw_list() const
{
    return mObj->drawList;
}

} // namespace LD