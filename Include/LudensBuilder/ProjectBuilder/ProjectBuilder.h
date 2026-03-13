#pragma once

#include <Ludens/Header/Error.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/System/FileSystemAsync.h>

#include <atomic>

namespace LD {

enum ProjectBuildErrorType
{
    PROJECT_BUILD_ERROR_IO,
    PROJECT_BUILD_ERROR_INVALID_SRC_PROJECT_SCHEMA,
    PROJECT_BUILD_ERROR_INVALID_SRC_ASSET_SCHEMA,
};

struct ProjectBuildError : TError<ProjectBuildErrorType>
{
};

struct ProjectBuildConfig
{
    FS::Path srcProjectSchema;
    FS::Path dstRootDirectory;
};

struct ProjectBuildResult
{
    bool success;

    void reset();
};

struct BuildProjectAsync : Handle<struct BuildProjectAsyncObj>
{
    static BuildProjectAsync create();

    static void destroy(BuildProjectAsync async);

    /// @brief Called by worker thread to begin build jobs
    /// @return True if the async build has started.
    bool begin(const ProjectBuildConfig& config, ProjectBuildError& err);

    /// @brief Atomically check build status.
    bool has_completed();

    /// @brief Get results after build completion.
    bool get_result(ProjectBuildResult& outResult);
};

} // namespace LD
