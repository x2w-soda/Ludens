#pragma once

#include <Ludens/DSA/Buffer.h>
#include <Ludens/DSA/View.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Header/Math/Transform.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Support for the glTF v2.0 format
// - Throughout this header, 'the spec' refers to the 2.0 version of the glTF file format specification as defined by the Khronos Group.
// - Throughout this header, the View struct is a UTF-8 encoded byte stream, and the data address should be assumed transient.
// - While the spec only suggests that generators SHOULD use UTF-8, this header strictly only supports UTF-8 parsing.

namespace LD {

/// @brief Top-level 'asset' property in the spec.
struct GLTFAssetProp
{
    Buffer version;
    Buffer generator;
    Buffer copyright;
};

/// @brief Element in top-level 'scenes' property in the spec.
struct GLTFSceneProp
{
    Buffer name;                 // authored scene name
    std::vector<uint32_t> nodes; // indices of nodes in this scene
};

/// @brief Element in top-level 'nodes' property in the spec.
struct GLTFNodeProp
{
    Buffer name;                    // authored node name
    std::optional<uint32_t> mesh;   // node.mesh, the index into top-level meshes array
    std::vector<uint32_t> children; // node.children, indices of children nodes
    Mat4 matrix;                    // node.matrix, a column major local transformation for the node
    Transform TRS;                  // node.translation, node.rotation, and node.scale
};

/// @brief Metallic-roughness model in the spec.
struct GLTFPbrMetallicRoughness
{
    Vec4 baseColorFactor = Vec4(1.0f); // defaults to [1.0, 1.0, 1.0, 1.0]
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
};

/// @brief Element in top-level 'materials' property in the spec.
struct GLTFMaterialProp
{
    Buffer name;                                 // authored material name
    std::optional<GLTFPbrMetallicRoughness> pbr; // material.pbrMetallicRoughness
    bool doubleSided = false;                    // material.doubleSided
};

struct GLTFEventCallback
{
    /// @brief Top-level 'asset' property in the spec.
    bool (*onAsset)(const GLTFAssetProp& asset, void* user);

    /// @brief Top-level 'scene' property in the spec, the index of scenes to render.
    bool (*onSceneIndex)(uint32_t sceneIdx, void* user);

    /// @brief Element in top-level 'scenes' property in the spec.
    bool (*onScene)(const GLTFSceneProp& scene, void* user);

    /// @brief Element in top-level 'nodes' property in the spec.
    bool (*onNode)(const GLTFNodeProp& node, void* user);

    /// @brief Element in top-level 'materials' property in the spec.
    bool (*onMaterial)(const GLTFMaterialProp& mat, void* user);
};

struct GLTFEventParser
{
    static bool parse(const View& file, std::string& error, const GLTFEventCallback& callbacks, void* user);
};

bool print_gltf_data(const View& file, std::string& str, std::string& err);

} // namespace LD