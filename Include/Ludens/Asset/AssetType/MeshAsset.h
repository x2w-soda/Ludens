#pragma once

#include <Ludens/Asset/Asset.h>
#include <Ludens/JobSystem/JobSystem.h>
#include <Ludens/Media/Model.h>
#include <filesystem>

namespace LD {

/// @brief Mesh asset handle.
struct MeshAsset : AssetHandle<struct MeshAssetObj>
{
    /// @brief get mesh binary data
    struct ModelBinary* data();

    /// @brief Unload asset from RAM. The handle becomes null afterwards.
    void unload();
};

struct MeshAssetImportInfo
{
    std::filesystem::path sourcePath; /// path to load the source format
    std::filesystem::path savePath;   /// path to save the imported asset
};

class MeshAssetImportJob
{
public:
    MeshAsset asset;          /// subject asset handle
    MeshAssetImportInfo info; /// mesh import configuration

    /// @brief Submit to job system. Address of this job instance must not
    ///        change until the worker thread completes execution.
    void submit();

private:
    static void execute(void*);

    JobHeader mHeader;
};

class MeshAssetLoadJob
{
public:
    MeshAsset asset;
    std::filesystem::path loadPath;

    /// @brief Submit to job system. Address of this job instance must not
    ///        change until the worker thread completes execution.
    void submit();

private:
    static void execute(void*);

    JobHeader mHeader;
};

} // namespace LD