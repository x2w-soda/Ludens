#pragma once

#include <Ludens/RenderBackend/RBackend.h>
#include <Ludens/Header/Math/Mat4.h>

#define LD_GLSL_FRAME_SET R"(
layout (set = 0, binding = 0) uniform frame {
    mat4 viewMat;
    mat4 projMat;
    mat4 viewProjMat;
    vec4 viewPos;
    vec4 dirLight;
    float width;
    float height;
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
    Mat4 viewMat;
    Mat4 projMat;
    Mat4 viewProjMat;
    Vec4 viewPos;
    Vec4 dirLight;
    float width;
    float height;
    float envPhase;
};

} // namespace LD