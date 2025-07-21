#pragma once

#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/RenderBackend/RBackend.h>

#define LD_GLSL_FRAME_SET R"(
layout (set = 0, binding = 0) uniform frame {
    mat4 viewMat;
    mat4 projMat;
    mat4 viewProjMat;
    vec4 viewPos;
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

struct FrameUBO
{
    Mat4 viewMat;      /// main camera view matrix
    Mat4 projMat;      /// main camera projection matrix
    Mat4 viewProjMat;  /// main camera view-projection matrix product
    Vec4 viewPos;      /// main camera view position
    Vec4 dirLight;     /// directional light
    Vec2 screenExtent; /// extent of the whole screen
    Vec2 sceneExtent;  /// extent of the scene
    float envPhase;    /// normalized environment map phase 0 to 1
};

} // namespace LD