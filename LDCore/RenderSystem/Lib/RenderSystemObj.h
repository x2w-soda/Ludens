#pragma once

#include <Ludens/DSA/HashSet.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/RenderBackend/RUID.h>
#include <Ludens/RenderComponent/Layout/RMesh.h>
#include <Ludens/RenderSystem/RenderSystem.h>

namespace LD {

class ScreenLayerObj;

struct Sprite2DDrawObj
{
    RUID id = 0;                     // draw identifier for this struct.
    uint32_t zDepth = 0;             // depth within layer.
    ScreenLayerObj* layer = nullptr; // link to current screen layer
    RImage image{};                  // image to render.
    Rect rect{};                     // sprite local geometry before transform is applied.
};

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