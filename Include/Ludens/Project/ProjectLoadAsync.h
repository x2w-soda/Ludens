#pragma once

#include <Ludens/Asset/AssetRegistry.h>
#include <Ludens/DSA/String.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Project/Project.h>

namespace LD {

class ProjectContext;

enum ProjectLoadState
{
    PROJECT_LOAD_STATE_IDLE = 0,
    PROJECT_LOAD_STATE_LOADING_ASSETS,
    PROJECT_LOAD_STATE_LOADING_SCENE,
    PROJECT_LOAD_STATE_COMPLETE,
};

/// @brief Control data for loading a project asynchronously.
///        Main thread only.
struct ProjectLoadAsync : Handle<struct ProjectLoadAsyncObj>
{
    static ProjectLoadAsync create(ProjectContext* projectCtx);
    static void destroy(ProjectLoadAsync async);

    /// @brief Try begin async project loading.
    bool begin(const FS::Path& projectDir, String& err);

    /// @brief Non-blocking update.
    ProjectLoadState update();
};

} // namespace LD