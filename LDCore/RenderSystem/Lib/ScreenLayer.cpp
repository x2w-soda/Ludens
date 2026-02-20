#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Mat3.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>

#include "RenderSystemObj.h"
#include "ScreenLayer.h"

#include <cstdint>
#include <cstring>

namespace LD {

static_assert(LD::IsTrivial<Sprite2DDrawObj>);
static_assert(LD::IsTrivial<ScreenLayerItem>);

ScreenLayerObj::ScreenLayerObj(RUID id, const std::string& name)
    : mID(id), mName(name)
{
    LD_ASSERT(mID && !mName.empty());

    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(Sprite2DDrawObj);
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

void ScreenLayerObj::invalidate(RenderSystemMat4Callback mat4CB, void* user)
{
    LD_PROFILE_SCOPE;

    mItemList.clear();

    // TODO: reserve item list by PoolAllocator size

    for (auto it = mSprite2DDrawPA.begin(); it; ++it)
    {
        auto* draw = (Sprite2DDrawObj*)it.data();

        ScreenLayerItem item;
        item.zDepth = draw->zDepth;
        item.type = SCREEN_LAYER_ITEM_SPRITE_2D;
        item.sprite2D = draw;
        mItemList.push_back(item);
    }

    sort_items();

    build_items(mat4CB, user);
}

RUID ScreenLayerObj::pick_item(const Vec2& worldPos, RenderSystemMat4Callback mat4CB, void* user)
{
    LD_PROFILE_SCOPE;

    for (const ScreenLayerItem& item : mItemList)
    {
        // broad phase
        float dx = item.sphereX - worldPos.x;
        float dy = item.sphereY - worldPos.y;
        if (dx * dx + dy * dy > item.sphereR2)
            continue;

        // narrow phase depends on actual geometry
        LD_ASSERT(item.type == SCREEN_LAYER_ITEM_SPRITE_2D);
        Mat4 modelMat;
        if (!mat4CB(item.sprite2D->id, modelMat, user))
            continue;

        Vec4 pickLocalPos = Mat4::inverse(modelMat) * Vec4(worldPos, 0.0f, 1.0f);
        Rect localPos, localUV;
        item.sprite2D->get_local(localPos, localUV);
        if (localPos.contains(Vec2(pickLocalPos.x, pickLocalPos.y)))
            return item.sprite2D->id;
    }

    return 0;
}

TView<ScreenLayerItem> ScreenLayerObj::get_item_list()
{
    return TView<ScreenLayerItem>(mItemList.data(), mItemList.size());
}

Sprite2DDrawObj* ScreenLayerObj::create_sprite_2d(RUID drawID, RImage image)
{
    auto* draw = (Sprite2DDrawObj*)mSprite2DDrawPA.allocate();
    draw->layer = this;
    draw->id = drawID;
    draw->region = {};
    draw->image = image;
    draw->zDepth = 0;
    draw->pivot = {};

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
    const uint32_t N = (uint32_t)mItemList.size();

    Vector<ScreenLayerItem> tmpItems(N);
    ScreenLayerItem* src = mItemList.data();
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
    LD_ASSERT(src == mItemList.data());
}

void ScreenLayerObj::build_items(RenderSystemMat4Callback mat4CB, void* user)
{
    LD_PROFILE_SCOPE;

    for (ScreenLayerItem& item : mItemList)
    {
        LD_ASSERT(item.type == SCREEN_LAYER_ITEM_SPRITE_2D);

        Mat4 modelMat;
        bool ok = mat4CB(item.sprite2D->id, modelMat, user);
        float scaleX2 = modelMat[0][0] * modelMat[0][0] + modelMat[0][1] * modelMat[0][1]; 
        float scaleY2 = modelMat[1][0] * modelMat[1][0] + modelMat[1][1] * modelMat[1][1]; 
        float halfW = item.sprite2D->region.w / 2.0f;
        float halfH = item.sprite2D->region.h / 2.0f;
        Vec4 sphereCenter = modelMat * Vec4(item.sprite2D->get_local_center(), 0.0f, 1.0f);
        item.sphereX = sphereCenter.x;
        item.sphereY = sphereCenter.y;
        item.sphereR2 = std::max(scaleX2, scaleY2) * ((halfW * halfW) + (halfH * halfH));
    }
}

} // namespace LD