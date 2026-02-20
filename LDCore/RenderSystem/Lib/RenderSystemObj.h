#pragma once

#include <Ludens/DSA/HashSet.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderBackend/RUID.h>
#include <Ludens/RenderComponent/Layout/RMesh.h>
#include <Ludens/RenderComponent/Layout/SetLayouts.h>
#include <Ludens/RenderSystem/RenderSystem.h>

namespace LD {

class ScreenLayerObj;

/// @brief High level intent to draw a sprite, iterated.
struct Sprite2DDrawObj
{
    RUID id;               // draw identifier for this struct
    ScreenLayerObj* layer; // link to current screen layer
    RImage image;          // image to render
    uint32_t zDepth;       // depth within layer
    Rect region;           // rendererd region in pixel space
    Vec2 pivot;            // pivot hint for scale and rotation

    inline void get_local(Rect& pos, Rect& uv) const
    {
        const float imageW = (float)image.width();
        const float imageH = (float)image.height();
        const float spriteW = std::min(imageW, region.w);
        const float spriteH = std::min(imageH, region.h);
        pos = Rect(-pivot.x, -pivot.y, spriteW, spriteH);
        uv = Rect(region.x / imageW, region.y / imageH, region.w / imageW, region.h / imageH);
    }

    inline Vec2 get_local_center()
    {
        const float imageW = (float)image.width();
        const float imageH = (float)image.height();
        const float spriteW = std::min(imageW, region.w);
        const float spriteH = std::min(imageH, region.h);
        return Vec2(-pivot.x + spriteW / 2.0f, -pivot.y + spriteH / 2.0f);
    }
};

/// @brief High level intent to draw a mesh, iterated.
struct MeshDataObj
{
    RUID id = 0;
    RMesh mesh{};
    HashSet<RUID> drawID;
};

struct MeshDrawObj
{
    RUID id = 0;
    MeshData data{};
};

} // namespace LD