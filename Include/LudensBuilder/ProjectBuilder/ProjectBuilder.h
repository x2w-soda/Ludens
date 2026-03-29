#pragma once

#include <Ludens/Header/Error.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/System/FileSystemAsync.h>

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

struct ProjectBuildAsync : Handle<struct ProjectBuildAsyncObj>
{
    static ProjectBuildAsync create();

    static void destroy(ProjectBuildAsync async);

    /// @brief Called by worker thread to begin build jobs
    /// @return True if the async build has started.
    bool begin(const ProjectBuildConfig& config, ProjectBuildError& err);

    /// @brief Atomically check build status.
    bool has_completed();

    /// @brief Get results after build completion.
    bool get_result(ProjectBuildResult& outResult);
};

/// @brief Tries to create empty project in destination directory.
///        This call is synchronous and generates default schema files.
bool create_empty_project(const std::string& projectName, const FS::Path& projectDir, std::string& err);

} // namespace LD
