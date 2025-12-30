#pragma once

#include <Ludens/DSA/Buffer.h>
#include <Ludens/DSA/Optional.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/DSA/View.h>
#include <Ludens/Header/Math/Mat4.h>
#include <Ludens/Header/Math/Transform.h>
#include <Ludens/Header/Math/Vec3.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>

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

/// @brief Element in top-level 'images' property in the spec.
struct GLTFImageProp
{
    Buffer name;                   // optional authored name for this image
    Buffer uri;                    // optional URI of the image
    Buffer mimeType;               // image media type, must be defined if bufferView is defined
    Optional<uint32_t> bufferView; // index of buffer view that contains the image, must not be defined if uri is defined
};

/// @brief Element in top-level 'buffers' property in the spec.
struct GLTFBufferProp
{
    Buffer name;
    Buffer uri;
    uint64_t byteLength;
};

/// @brief Element in top-level 'bufferViews' property in the spec.
struct GLTFBufferViewProp
{
    Buffer name;                   // authored name for this view
    uint32_t buffer;               // index into 'buffers' array
    uint64_t byteOffset = 0;       // byte offset into subject buffer
    uint64_t byteLength;           // length of the view in bytes
    Optional<uint64_t> byteStride; // byte stride, data is tightly packed if not defined
    Optional<uint32_t> target;     // hint representing the intended GPU buffer type to use with this buffer view
};

/// @brief Element in top-level 'accessors' property in the spec.
struct GLTFAccessorProp
{
    Buffer name;                   // authored name for this accessor
    Buffer type;                   // required, specifies element type, must be one of "SCALAR", "VEC2", "VEC3", "VEC4", "MAT2", "MAT3", "MAT4"
    uint64_t byteOffset = 0;       // additional offset applied after bufferView.byteOffset, must be a multiple of the size of the componentType
    uint32_t componentType = 0;    // required, data type of the accessor's components
    uint32_t count = 0;            // required, number of elements references by this accessor
    Optional<uint32_t> bufferView; // index of buffer view. When undefined, the accessor must be initialized with zeros
    Vector<float> min;             // minimum value of each component in this accessor, length is determined by type property
    Vector<float> max;             // maximum value of each component in this accessor, length is determined by type property
    bool normalized = false;       // specifies whether unsigned types are normalized to [0, 1] and signed types to [-1, 1] when they are accessed.
};

/// @brief Element in top-level 'samplers' property in spec.
struct GLTFSamplerProp
{
    Buffer name;                  // optional authored name for this sampler
    Optional<uint32_t> magFilter; // optional magnification filter
    Optional<uint32_t> minFilter; // optional minification filter
    uint32_t wrapS = 10497;       // optional S (U) wrapping mode
    uint32_t wrapT = 10497;       // optional T (V) wrapping mode
};

/// @brief Element in top-level 'textures' property in spec.
struct GLTFTextureProp
{
    Buffer name;                // optional authored name for this texture
    Optional<uint32_t> sampler; // optional sampler
    Optional<uint32_t> source;  // optional image
};

/// @brief Element in top-level 'scenes' property in the spec.
struct GLTFSceneProp
{
    Buffer name;            // authored scene name
    Vector<uint32_t> nodes; // indices of nodes in this scene
};

/// @brief Element in top-level 'nodes' property in the spec.
struct GLTFNodeProp
{
    Buffer name;               // authored node name
    Optional<uint32_t> mesh;   // node.mesh, the index into top-level meshes array
    Vector<uint32_t> children; // node.children, indices of children nodes
    Mat4 matrix;               // node.matrix, a column major local transformation for the node
    Transform TRS;             // node.translation, node.rotation, and node.scale
};

/// @brief 'mesh.primitive' property in the spec.
struct GLTFMeshPrimitiveProp
{
    std::unordered_map<Buffer, uint32_t> attributes;
    Optional<uint32_t> indices;  // index of accessor that contains vertex indices
    Optional<uint32_t> material; // index of material used for this primitive
    uint32_t mode = 4;           // topology of primitives
};

/// @brief Element in top-level 'meshes' property in the spec.
struct GLTFMeshProp
{
    Buffer name;
};

/// @brief 'textureInfo' in the spec.
struct GLTFTextureInfo
{
    uint32_t index;        // index into top-level 'textures' array
    uint32_t texCoord = 0; // set index of texture's TEXCOORD attribute used, defaults to 0
};

/// @brief 'material.normalTextureInfo' in the spec.
struct GLTFNormalTextureInfo : GLTFTextureInfo
{
    float scale = 1.0f;
};

/// @brief 'material.occlusionTextureInfo' in the spec.
struct GLTFOcclusionTextureInfo : GLTFTextureInfo
{
    float strength = 1.0f;
};

/// @brief Metallic-roughness model in the spec.
struct GLTFPbrMetallicRoughness
{
    Vec4 baseColorFactor = Vec4(1.0f); // defaults to [1.0, 1.0, 1.0, 1.0]
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    Optional<GLTFTextureInfo> baseColorTexture;
    Optional<GLTFTextureInfo> metallicRoughnessTexture;
};

/// @brief Element in top-level 'materials' property in the spec.
struct GLTFMaterialProp
{
    Buffer name;                                         // authored material name
    Optional<GLTFPbrMetallicRoughness> pbr;              // material.pbrMetallicRoughness
    Optional<GLTFNormalTextureInfo> normalTexture;       // material.normalTextureInfo
    Optional<GLTFOcclusionTextureInfo> occlusionTexture; // material.occlusionTextureInfo
    Optional<GLTFTextureInfo> emissiveTexture;           // material.textureInfo
    Vec3 emissiveFactor = Vec3(0.0f);                    // material.emissiveFactor
    bool doubleSided = false;                            // material.doubleSided
    float alphaCutoff = 0.5f;                            // material.alphaCutoff
    Buffer alphaMode = "OPAQUE";                         // material.alphaMode
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

    /// @brief Element in 'mesh.primitives' property in a mesh.
    bool (*onMeshPrimitive)(const GLTFMeshPrimitiveProp& prim, void* user);

    /// @brief Element in top-level 'meshes' property in the spec. Previous MeshPrimitive properties
    ///        all belong to this mesh, upcoming MeshPrimitives belong to next mesh.
    bool (*onMesh)(const GLTFMeshProp& mesh, void* user);

    /// @brief Element in top-level 'materials' property in the spec.
    bool (*onMaterial)(const GLTFMaterialProp& mat, void* user);

    /// @brief Element in top-level 'textures' property in the spec.
    bool (*onTexture)(const GLTFTextureProp& texture, void* user);

    /// @brief Element in top-level 'samplers' property in the spec.
    bool (*onSampler)(const GLTFSamplerProp& sampler, void* user);

    /// @brief Element in top-level 'images' property in the spec.
    bool (*onImage)(const GLTFImageProp& image, void* user);

    /// @brief Element in top-level 'buffers' property in the spec.
    bool (*onBuffer)(const GLTFBufferProp& buf, void* user);

    /// @brief Element in top-level 'bufferViews' property in the spec.
    bool (*onBufferView)(const GLTFBufferViewProp& buf, void* user);

    /// @brief Element in top-level 'accessors' property in the spec.
    bool (*onAccessor)(const GLTFAccessorProp& accessor, void* user);
};

struct GLTFEventParser
{
    static bool parse(const View& file, std::string& error, const GLTFEventCallback& callbacks, void* user);
};

bool print_gltf_data(const View& file, std::string& str, std::string& err);

} // namespace LD