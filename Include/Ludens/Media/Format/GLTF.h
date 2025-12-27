#pragma once

#include <Ludens/DSA/Buffer.h>
#include <Ludens/DSA/View.h>

#include <cstddef>
#include <cstdint>
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
struct GLTFScenesProp
{
    Buffer name;
    std::vector<uint32_t> nodes;
};

struct GLTFEventCallback
{
    bool (*onAsset)(const GLTFAssetProp& asset, void* user);

    /// @brief Top-level 'scene' property in the spec, the index of scenes to render.
    bool (*onSceneProp)(uint32_t sceneIdx, void* user);

    /// @brief Element in top-level 'scenes' property in the spec.
    bool (*onScenes)(const GLTFScenesProp& scene, void* user);
};

struct GLTFEventParser
{
    static bool parse(const View& file, std::string& error, const GLTFEventCallback& callbacks, void* user);
};

bool print_gltf_data(const View& file, std::string& str, std::string& err);

} // namespace LD