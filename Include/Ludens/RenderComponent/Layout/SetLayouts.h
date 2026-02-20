#pragma once

#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Header/Math/Viewport.h>
#include <Ludens/RenderBackend/RBackend.h>

#define LD_GLSL_FRAME_SET R"(
struct ViewProjectionData
{
    mat4 viewMat;
    mat4 projMat;
    mat4 viewProjMat;
    vec4 viewPos;
};
layout (set = 0, binding = 0) uniform frame {
    ViewProjectionData vp[24];
    vec4 dirLight;
    vec2 screenExtent;
    vec2 sceneExtent;
    float envPhase;
} uFrame;
layout (set = 0, binding = 1) uniform samplerCube uEnv;
)"

namespace LD {

/// @brief the layout of the frame set that is statically bound
///        at index 0 throughout the entire frame.
extern RSetLayoutInfo sFrameSetLayout;

/// @brief the layout of the bindings required by a material
extern RSetLayoutInfo sMaterialSetLayout;

/// @brief a common layout with a single sampled image at binding 0
extern RSetLayoutInfo sSingleSampleSetLayout;

/// @brief a common layout with two sampled images at binding 0 and 1
extern RSetLayoutInfo sDoubleSampleSetLayout;

/// @brief GPU-side view and projection information.
struct ViewProjectionData
{
    Mat4 viewMat;
    Mat4 projMat;
    Mat4 viewProjMat;
    Vec4 viewPos;

    ViewProjectionData() = default;
    ViewProjectionData(const Mat4& view, const Mat4& proj, const Vec4& viewPos)
        : viewMat(view), projMat(proj), viewPos(viewPos)
    {
        viewProjMat = proj * view;
    }

    static ViewProjectionData from_viewport(const Viewport& viewport)
    {
        return ViewProjectionData(viewport.viewMat, viewport.projMat, viewport.viewPos);
    }
};

struct FrameUBO
{
    ViewProjectionData vp[24]; /// arbitrary view projections
    Vec4 dirLight;             /// directional light
    Vec2 screenExtent;         /// extent of the whole screen
    Vec2 sceneExtent;          /// extent of the scene
    float envPhase;            /// normalized environment map phase 0 to 1
};

class FrameUBOManager
{
public:
    void reset(const Vec2& screenExntent, const Vec2& sceneExtent);

    /// @brief Get current frame UBO data.
    inline const FrameUBO* get() { return &mUBO; }

    /// @brief Register a view projection data.
    /// @return Non-negative index into VP array on success.
    int register_vp(const ViewProjectionData& vp);

private:
    FrameUBO mUBO;
    int mVPIndex;
};

} // namespace LD