#pragma once
#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Color.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Header/View.h>
#include <Ludens/Memory/Allocator.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderSystem/RenderSystem.h>

#include <string>

namespace LD {

struct Transform2D;

struct ScreenLayerItem
{
    uint32_t zDepth;
    Color color;
    RImage image;
    Vec2 tl, tr, bl, br;
};

class ScreenLayerObj
{
public:
    ScreenLayerObj() = delete;
    ScreenLayerObj(RUID id, const std::string& name);
    ScreenLayerObj(const ScreenLayerObj&) = delete;
    ~ScreenLayerObj();

    ScreenLayerObj& operator=(const ScreenLayerObj&) = delete;

    /// @brief Force invalidate draw list. This sorts all 2D items by Z depth.
    void invalidate(RenderSystemMat4Callback mat4Callback, void* user);

    TView<ScreenLayerItem> get_draw_list();
    inline void set_name(const std::string& name) { mName = name; }
    inline std::string get_name() { return mName; }
    inline RUID get_id() { return mID; }

    /// @brief Create sprite 2d in layer, subsequent modifications can be done through Sprite2DDrawObj directly,
    //         changes are reflected after the next invalidate().
    Sprite2DDrawObj* create_sprite_2d(RUID drawID, const Rect& rect, RImage image, uint32_t zDepth);

    void destroy_sprite_2d(Sprite2DDrawObj* draw);

private:
    void sort_items();

private:
    RUID mID = 0;
    Vector<ScreenLayerItem> mDrawList;
    PoolAllocator mSprite2DDrawPA{};
    std::string mName;
};


} // namespace LD