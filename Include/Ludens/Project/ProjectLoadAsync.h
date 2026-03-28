#pragma once

#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Project/Project.h>

#include <string>

namespace LD {

enum ProjectLoadStatus
{
    PROJECT_LOAD_STATUS_IDLE = 0,
    PROJECT_LOAD_STATUS_LOADING_ASSETS,
    PROJECT_LOAD_STATUS_LOADING_SCENE,
    PROJECT_LOAD_STATUS_COMPLETE,
};

struct ProjectLoadResult
{
    Project project = {};             // if not null, the loaded project from ProjectSchema
    AssetRegistry assetRegistry = {}; // if not null, the loaded asset registry from AssetSchema
    FS::Path assetSchemaPath;         // absolute path to the project schema file
    FS::Path sceneSchemaPath;         // absolute path to the asset schema file
};

/// @brief Control data for loading a project asynchronously.
///        Main thread only.
struct ProjectLoadAsync : Handle<struct ProjectLoadAsyncObj>
{
    static ProjectLoadAsync create();
    static void destroy(ProjectLoadAsync async);

    /// @brief Try begin async project loading.
    bool begin(const FS::Path& projectDir, std::string& err);

    /// @brief Non-blocking update.
    ProjectLoadStatus update();

    /// @brief After completion, retrieve results.
    bool get_result(ProjectLoadResult& outResult);
};

} // namespace LD