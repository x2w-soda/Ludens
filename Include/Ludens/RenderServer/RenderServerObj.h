#pragma once

#include <Ludens/DSA/HashSet.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/IDHandle.h>
#include <Ludens/RenderBackend/RUID.h>
#include <Ludens/RenderComponent/Layout/RMesh.h>

#include <cstdint>

namespace LD {

// Render Server Objects with exposed memory layouts.
// - Object address is persistent and does not change throughout Object lifetime.
// - Any two objects are guaranteed to have distinct RUID.
// The two invariants above allow us to use IDHandle and raw pointer access
// User Responsibility:
// - Objects are externally synchronized, the RenderServer user is responsible for syncrhonizing Object access.

class RenderServerObj;
class ScreenLayerObj;
struct Sprite2DDrawObj;
struct MeshDataObj;
struct MeshDrawObj;
struct RImageObj;

using Image2D = IDHandle<RImageObj, RUID>;
using ImageCube = IDHandle<RImageObj, RUID>;

struct Sprite2DDrawObj
{
    RUID id = 0;                     // draw identifier for this struct.
    uint32_t zDepth = 0;             // depth within layer.
    ScreenLayerObj* layer = nullptr; // link to current screen layer
    RImage image{};                  // image to render.
    Rect rect{};                     // sprite local geometry before transform is applied.
};

struct Sprite2DDraw : IDHandle<Sprite2DDrawObj, RUID>
{
    Sprite2DDraw() = default;
    Sprite2DDraw(Sprite2DDrawObj* obj, RUID id)
        : IDHandle(obj, id) {}

    inline void set_image(Image2D image2D)
    {
        if (!image2D)
            return;

        mObj->image = RImage(image2D.unwrap());
    }

    inline uint32_t get_z_depth()
    {
        return mObj->zDepth;
    }

    inline void set_z_depth(uint32_t zDepth)
    {
        mObj->zDepth = zDepth;
    }

    inline Rect get_rect()
    {
        return mObj->rect;
    }

    inline void set_rect(const Rect& rect)
    {
        mObj->rect = rect;
    }
};

struct MeshDataObj
{
    RUID id = 0;
    RMesh mesh{};
    HashSet<RUID> drawID;
};

struct MeshData : IDHandle<MeshDataObj, RUID>
{
    MeshData() = default;
    MeshData(MeshDataObj* obj, RUID id)
        : IDHandle(obj, id) {}
};

struct MeshDrawObj
{
    RUID id = 0;
    MeshData data{};
};

struct MeshDraw : IDHandle<MeshDrawObj, RUID>
{
    MeshDraw() = default;
    MeshDraw(MeshDrawObj* obj, RUID id)
        : IDHandle(obj, id) {}

    inline void set_mesh_asset(MeshData data)
    {
        if (!data)
            return;

        if (mObj->data)
            mObj->data.unwrap()->drawID.erase(mID);

        mObj->data = data;

        MeshDataObj* dataObj = data.unwrap();
        dataObj->drawID.insert(mID);
    }
};

} // namespace LD