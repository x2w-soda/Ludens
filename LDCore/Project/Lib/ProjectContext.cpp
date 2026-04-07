#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/Project/ProjectContext.h>
#include <Ludens/Project/ProjectSchema.h>

namespace LD {

ProjectContext::ProjectContext()
{
    mProject = Project::create();
    mAssetRegistry = AssetRegistry::create();
    mSUIDRegistry = SUIDRegistry::create();
}

ProjectContext::~ProjectContext()
{
    Project::destroy(mProject);
    AssetRegistry::destroy(mAssetRegistry);
    SUIDRegistry::destroy(mSUIDRegistry);
}

bool ProjectContext::load_project(const FS::Path& projectSchemaPath, std::string& err)
{
    if (!ProjectSchema::load_project_from_file(mProject, mSUIDRegistry, projectSchemaPath, err))
        return false;

    return true;
}

bool ProjectContext::load_asset_registry(const FS::Path& assetSchemaPath, std::string& err)
{
    if (!AssetSchema::load_registry_from_file(mAssetRegistry, mSUIDRegistry, assetSchemaPath, err))
        return false;

    return true;
}

bool ProjectContext::save_project(std::string err)
{
    if (!ProjectSchema::save_project(mProject, mProject.get_project_schema_path(), err))
        return false;

    return true;
}

bool ProjectContext::save_asset_registry(std::string err)
{
    if (!AssetSchema::save_registry(mAssetRegistry, mProject.get_asset_schema_absolute_path(), err))
        return false;

    return true;
}

Vector<FS::Path> ProjectContext::scene_schema_abs_paths()
{
    Vector<FS::Path> paths;

    mProject.get_scene_schema_absolute_paths(paths);

    return paths;
}

} // namespace LD