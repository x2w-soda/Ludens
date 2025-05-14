#pragma once

#include <Ludens/Header/Math/Vec4.h>
#include <Ludens/RenderBackend/RBackend.h>

namespace LD {

#define LD_GLSL_MATERIAL_SET(IDX) R"(
layout (set = )" #IDX R"(, binding = 0) uniform Mat {
    vec4 colorFactor;
    float metallicFactor;
    float roughnessFactor;
    uint hasColorTexture;
    uint hasNormalTexture;
    uint hasMetallicRoughnessTexture;
} uMat;
layout (set = )" #IDX R"(, binding = 1) uniform sampler2D uMatColor;
layout (set = )" #IDX R"(, binding = 2) uniform sampler2D uMatNormal;
layout (set = )" #IDX R"(, binding = 3) uniform sampler2D uMatMetallicRoughness;
)"

/// @brief material parameters in the form of a uniform buffer
struct RMaterialUBO
{
    Vec4 colorFactor;
    float metallicFactor;
    float roughnessFactor;
    uint32_t hasColorTexture;
    uint32_t hasNormalTexture;
    uint32_t hasMetallicRoughnessTexture;
};

/// @brief renderer friendly layout of a Material
struct RMaterial
{
    RSet set;    /// binds a RMaterialUBO and material textures
    RBuffer ubo; /// RMaterialUBO on the GPU
};

} // namespace LD