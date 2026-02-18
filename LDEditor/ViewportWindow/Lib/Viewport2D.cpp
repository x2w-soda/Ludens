#include "Viewport2D.h"

namespace LD {

Viewport2D::~Viewport2D()
{
    LD_ASSERT(!mCamera);
}

void Viewport2D::create(const Vec2& sceneExtent)
{
    mCamera = Camera2D::create(Camera2DInfo::extent(sceneExtent.x, sceneExtent.y));
}

void Viewport2D::destroy()
{
    if (mCamera)
    {
        Camera2D::destroy(mCamera);
        mCamera = {};
    }
}

void Viewport2D::imgui(const ViewportState& state)
{
    // TODO:
}

} // namespace LD