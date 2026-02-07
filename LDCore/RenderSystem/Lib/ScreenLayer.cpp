#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Mat3.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>

#include "ScreenLayer.h"

#include <cstdint>
#include <cstring>

namespace LD {

ScreenLayerObj::ScreenLayerObj(RUID id, const std::string& name)
    : mID(id), mName(name)
{
    LD_ASSERT(mID && !mName.empty());

    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(Sprite2DDraw);
    paI.pageSize = 256; // TODO: parameterize
    paI.usage = MEMORY_USAGE_RENDER;
    paI.isMultiPage = true;
    mSprite2DDrawPA = PoolAllocator::create(paI);
}

ScreenLayerObj::~ScreenLayerObj()
{
    if (mSprite2DDrawPA)
    {
        for (auto it = mSprite2DDrawPA.begin(); it; ++it)
        {
            auto* draw = (Sprite2DDrawObj*)it.data();

            draw->~Sprite2DDrawObj();
            draw->id = 0; // invalidates all Sprite2DDraw references
        }

        PoolAllocator::destroy(mSprite2DDrawPA);
    }
}

void ScreenLayerObj::invalidate(RenderSystemMat4Callback mat4Callback, void* user)
{
    LD_PROFILE_SCOPE;

    // TODO: This is still a lot of CPU overhead despite PoolAllocator providing cache-friendly iteration.
    //       Maybe Model matrix could be uploaded as UBO for ScreenRenderComponent?
    for (auto it = mSprite2DDrawPA.begin(); it; ++it)
    {
        auto* draw = (Sprite2DDrawObj*)it.data();

        if (!draw->image)
            continue;

        Mat4 modelMat = mat4Callback(draw->id, user);
        Vec4 tl = modelMat * Vec4(draw->rect.get_pos(), 0.0f, 1.0f);
        Vec4 tr = modelMat * Vec4(draw->rect.get_pos_tr(), 0.0f, 1.0f);
        Vec4 br = modelMat * Vec4(draw->rect.get_pos_br(), 0.0f, 1.0f);
        Vec4 bl = modelMat * Vec4(draw->rect.get_pos_bl(), 0.0f, 1.0f);

        ScreenLayerItem item;
        item.image = draw->image;
        item.color = Color(0xFFFFFFFF);
        item.zDepth = draw->zDepth;
        item.tl = Vec2(tl.x / tl.w, tl.y / tl.w);
        item.tr = Vec2(tr.x / tr.w, tr.y / tr.w);
        item.br = Vec2(br.x / br.w, br.y / br.w);
        item.bl = Vec2(bl.x / bl.w, bl.y / bl.w);
        mDrawList.push_back(item);
    }

    sort_items();
}

TView<ScreenLayerItem> ScreenLayerObj::get_draw_list()
{
    return TView<ScreenLayerItem>(mDrawList.data(), mDrawList.size());
}

Sprite2DDrawObj* ScreenLayerObj::create_sprite_2d(RUID drawID, const Rect& rect, RImage image, uint32_t zDepth)
{
    auto* draw = (Sprite2DDrawObj*)mSprite2DDrawPA.allocate();
    draw->layer = this;
    draw->id = drawID;
    draw->rect = rect;
    draw->image = image;
    draw->zDepth = zDepth;

    return draw;
}

void ScreenLayerObj::destroy_sprite_2d(Sprite2DDrawObj* draw)
{
    draw->id = 0;

    mSprite2DDrawPA.free(draw);
}

/// @brief Linear time radix sort by u32 depth.
void ScreenLayerObj::sort_items()
{
    LD_PROFILE_SCOPE;

    constexpr uint32_t R = 256;
    constexpr uint32_t mask = R - 1;
    const uint32_t N = (uint32_t)mDrawList.size();

    Vector<ScreenLayerItem> tmpItems(N);
    ScreenLayerItem* src = mDrawList.data();
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
    LD_ASSERT(src == mDrawList.data());
}

} // namespace LD