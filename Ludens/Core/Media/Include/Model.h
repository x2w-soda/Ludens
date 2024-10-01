#pragma once

#include <utility>
#include <string>
#include "Core/Header/Include/Types.h"
#include "Core/Math/Include/Hex.h"
#include "Core/Math/Include/Vec3.h"
#include "Core/DSA/Include/Vector.h"
#include "Core/DSA/Include/Optional.h"
#include "Core/OS/Include/JobSystem.h"
#include "Core/OS/Include/Memory.h"
#include "Core/IO/Include/FileSystem.h"
#include "Core/Media/Include/Mesh.h"
#include "Core/Media/Include/Image.h"

namespace LD
{

/// Material plain-old-data, non-programmable
struct Material
{
    Vec4 Albedo;
    float Roughness;
    float Metallic;
    Ref<Image> AlbedoTexture;
    Ref<Image> NormalTexture;
    Ref<Image> RoughnessTexture;
    Ref<Image> MetallicTexture;
    Ref<Image> MetallicRoughnessTexture;

    static Material GetDefault()
    {
        Vec4 pearlWhite = Hex(0xF8F6F0FF);

        Material mtl;
        mtl.Albedo = pearlWhite;
        mtl.Roughness = 0.1f;
        mtl.Metallic = 0.0f;
        mtl.AlbedoTexture = nullptr;
        mtl.NormalTexture = nullptr;
        mtl.RoughnessTexture = nullptr;
        mtl.MetallicTexture = nullptr;
        mtl.MetallicRoughnessTexture = nullptr;

        return mtl;
    }
};

/// plain-old-data for a Model, will be further processed by the renderer or physics engine
struct Model
{
    using MaterialRef = int;
    using MeshRefs = Vector<int>;

    /// each Mesh references a single material
    Vector<std::pair<Mesh, MaterialRef>> Meshes;

    /// each Material is used by one or more meshes
    Vector<std::pair<Material, MeshRefs>> Materials;
};

class ModelLoader
{
public:
    ModelLoader();
    ModelLoader(const ModelLoader&) = delete;
    ~ModelLoader();

    ModelLoader& operator=(const ModelLoader&) = delete;

    Ref<Model> LoadModel(const Path& path);

private:
};

class LoadModelJob
{
public:
    LoadModelJob() = delete;
    LoadModelJob(const LoadModelJob&) = delete;
    LoadModelJob(const Path& path, Ref<Model>* model);
    ~LoadModelJob() = default;

    LoadModelJob& operator=(const LoadModelJob&) = delete;

    /// get loading time on the worker thread in milliseconds
    double GetLoadTime()
    {
        LD_DEBUG_ASSERT(mHasCompleted);
        return mLoadTimeMS;
    }

private:
    static void JobMain(void* data);

    bool mHasCompleted = false;
    double mLoadTimeMS;
    Path mPath;
    Ref<Model>* mModel;
    ModelLoader mLoader;
};

} // namespace LD