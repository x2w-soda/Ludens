#include <Ludens/Asset/AssetSchema.h>
#include <Ludens/Project/ProjectContext.h>
#include <Ludens/Project/ProjectSchema.h>
#include <Ludens/Scene/Scene.h>

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

bool ProjectContext::load_project_schema(const FS::Path& projectSchemaPath, std::string& err)
{
    if (!ProjectSchema::load_project_from_file(mProject, mSUIDRegistry, projectSchemaPath, err))
        return false;

    return true;
}

bool ProjectContext::load_asset_schema(const FS::Path& assetSchemaPath, std::string& err)
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

void ProjectContext::configure_project_screen_layers()
{
    Scene scene = Scene::get();
    if (!scene)
        return;

    SceneRenderSystem renderS = scene.render_system();
    Vector<ProjectScreenLayer> layers = mProject.settings().screen_layer_settings().get_layers();
    Vector<SUID> ids(layers.size());
    Vector<std::string> names(layers.size());

    for (size_t i = 0; i < layers.size(); i++)
    {
        ids[i] = layers[i].id;
        names[i] = layers[i].name;
    }

    renderS.configure_screen_layers(layers.size(), ids.data(), names.data());
}

Vector<FS::Path> ProjectContext::scene_schema_abs_paths()
{
    Vector<FS::Path> paths;

    mProject.get_scene_schema_absolute_paths(paths);

    return paths;
}

FS::Path ProjectContext::default_scene_schema_abs_path()
{
    ProjectSceneEntry entry;
    if (!mProject.get_default_scene(entry))
        return {};

    return FS::absolute(mProject.get_root_path() / entry.path);
}

} // namespace LD