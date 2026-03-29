#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/System/FileSystem.h>

namespace LD {

enum ProjectScanStatus
{
    PROJECT_SCAN_STATUS_IDLE,
    PROJECT_SCAN_STATUS_PROJECT_SCHEMA,
    PROJECT_SCAN_STATUS_COMPLETE,
};

struct ProjectScanResult
{
    FS::Path projectSchema;
    std::string projectName;
    bool isProjectSchemaValid;
};

/// @brief Control data for scanning a project asynchronously.
///        Main thread only.
struct ProjectScanAsync : Handle<struct ProjectScanAsyncObj>
{
    static ProjectScanAsync create();
    static void destroy(ProjectScanAsync async);

    /// @brief Begin async project scan.
    void begin(const FS::Path& projectSchema);

    /// @brief Non-blocking update.
    ProjectScanStatus update();

    /// @brief After completion, retrieve results.
    bool get_result(ProjectScanResult& outResult);
};

} // namespace LD