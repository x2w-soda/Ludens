#pragma once

#include <Ludens/DSA/HashSet.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderBackend/RUID.h>
#include <Ludens/RenderComponent/Layout/RMesh.h>
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