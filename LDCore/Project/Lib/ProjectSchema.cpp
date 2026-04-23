#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Version.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Project/ProjectSchema.h>
#include <Ludens/System/FileSystem.h>

#include <format>

#include "ProjectSchemaKeys.h"
#include "ProjectSettingsDefault.h"

namespace LD {

/// @brief Saves a Project to TOML schema.
class ProjectSchemaSaver
{
public:
    ProjectSchemaSaver() = default;
    ProjectSchemaSaver(const ProjectSchemaSaver&) = delete;
    ~ProjectSchemaSaver();

    ProjectSchemaSaver& operator=(const ProjectSchemaSaver&) = delete;

    bool save_project(Project project, std::string& toml, std::string& err);

private:
    void save_project_settings(ProjectSettings settings, TOMLWriter writer);
    void save_project_startup_settings(ProjectStartupSettings settings, TOMLWriter writer);
    void save_project_screen_layer_settings(ProjectScreenLayerSettings settings, TOMLWriter writer);

private:
    Project mProject{};
    TOMLWriter mWriter{};
};

/// @brief Loads a Project from TOML schema.
class ProjectSchemaLoader
{
public:
    ProjectSchemaLoader() = default;
    ProjectSchemaLoader(const ProjectSchemaLoader&) = delete;
    ~ProjectSchemaLoader();

    ProjectSchemaLoader& operator=(const ProjectSchemaLoader&) = delete;

    bool load_project_schema(Project project, SUIDRegistry idReg, const View& toml, std::string& err);

private:
    bool load_project_settings(ProjectSettings settings, std::string& err);
    void load_project_startup_settings(ProjectStartupSettings settings);
    bool load_project_screen_layer_settings(ProjectScreenLayerSettings settings);

private:
    Project mProject{};
    TOMLReader mReader{};
    SUIDRegistry mIDReg{};
};

ProjectSchemaLoader::~ProjectSchemaLoader()
{
    if (mReader)
        TOMLReader::destroy(mReader);
}

bool ProjectSchemaLoader::load_project_schema(Project project, SUIDRegistry idReg, const View& toml, std::string& err)
{
    mReader = TOMLReader::create(toml, err);
    mProject = project;
    mIDReg = idReg;

    if (!mReader || !mReader.enter_table(PROJECT_SCHEMA_KEY_LUDENS_PROJECT))
        return false;

    uint32_t versionMajor;
    if (!mReader.read_u32(PROJECT_SCHEMA_KEY_VERSION_MAJOR, versionMajor) || versionMajor != LD_VERSION_MAJOR)
        return false;

    uint32_t versionMinor;
    if (!mReader.read_u32(PROJECT_SCHEMA_KEY_VERSION_MINOR, versionMinor) || versionMinor != LD_VERSION_MINOR)
        return false;

    uint32_t versionPatch;
    if (!mReader.read_u32(PROJECT_SCHEMA_KEY_VERSION_PATCH, versionPatch) || versionPatch != LD_VERSION_PATCH)
        return false;

    std::string str;
    if (!mReader.read_string(PROJECT_SCHEMA_KEY_NAME, str))
        return false;

    project.set_name(str);

    if (!mReader.read_string(PROJECT_SCHEMA_KEY_ASSETS, str))
        return false;

    mReader.exit();

    project.set_asset_schema_rel_path(FS::Path(str));

    bool isValid = true;
    uint32_t u32;
    std::string uriPath;
    int sceneCount = 0;
    if (mReader.enter_array(PROJECT_SCHEMA_KEY_SCENE, sceneCount))
    {
        for (int i = 0; isValid && i < sceneCount; i++)
        {
            if (!mReader.enter_table(i))
                continue; // ignore

            if (!mReader.read_string(PROJECT_SCHEMA_KEY_SCENE_PATH, uriPath))
            {
                mReader.exit();
                continue;
            }

            if (!mReader.read_u32(PROJECT_SCHEMA_KEY_SCENE_ID, u32))
            {
                mReader.exit();
                continue;
            }

            // conflicting scene entry makes the entire project schema invalid
            if (!project.register_scene_with_id(mIDReg, SUID(u32), uriPath, err))
                isValid = false;

            mReader.exit();
        }
        mReader.exit();
    }

    if (!isValid)
        return false;

    if (mReader.enter_table(PROJECT_SCHEMA_TABLE_SETTINGS))
    {
        isValid = load_project_settings(project.settings(), err);
        mReader.exit();
    }

    TOMLReader::destroy(mReader);
    mReader = {};

    return isValid;
}

bool ProjectSchemaLoader::load_project_settings(ProjectSettings settings, std::string& err)
{
    if (mReader.enter_table(PROJECT_SCHEMA_TABLE_STARTUP))
    {
        ProjectStartupSettings startup = settings.startup_settings();
        load_project_startup_settings(startup);
        mReader.exit();

        SUID dftSceneID = startup.get_default_scene_id();
        if (!mProject.has_scene(dftSceneID))
        {
            err = std::format("project startup settings: default_scene_id 0x{} does not exist", dftSceneID.to_string());
            return false;
        }
    }

    if (mReader.enter_table(PROJECT_SCHEMA_TABLE_SCREEN_LAYER))
    {
        ProjectScreenLayerSettings screenLayer = settings.screen_layer_settings();
        load_project_screen_layer_settings(screenLayer);
        mReader.exit();
    }

    return true;
}

void ProjectSchemaLoader::load_project_startup_settings(ProjectStartupSettings settings)
{
    uint32_t windowWidth = DEFAULT_STARTUP_WINDOW_WIDTH;
    mReader.read_u32(PROJECT_SCHEMA_KEY_STARTUP_WINDOW_WIDTH, windowWidth);
    settings.set_window_width(windowWidth);

    uint32_t windowHeight = DEFAULT_STARTUP_WINDOW_HEIGHT;
    mReader.read_u32(PROJECT_SCHEMA_KEY_STARTUP_WINDOW_HEIGHT, windowHeight);
    settings.set_window_height(windowHeight);

    std::string windowName = DEFAULT_STARTUP_WINDOW_NAME;
    mReader.read_string(PROJECT_SCHEMA_KEY_STARTUP_WINDOW_NAME, windowName);
    settings.set_window_name(windowName);

    uint32_t u32 = 0;
    mReader.read_u32(PROJECT_SCHEMA_KEY_DEFAULT_SCENE_ID, u32);
    settings.set_default_scene_id((SUID)u32);
}

bool ProjectSchemaLoader::load_project_screen_layer_settings(ProjectScreenLayerSettings settings)
{
    int count;
    if (!mReader.enter_array(PROJECT_SCHEMA_KEY_SCREEN_LAYERS, count))
        return false; // at least one screen layer is required

    for (int i = 0; i < count; i++)
    {
        if (!mReader.enter_table(i))
            continue;

        SUID suid;
        std::string str;
        mReader.read_suid(PROJECT_SCHEMA_KEY_SCREEN_LAYER_ID, suid);
        mReader.read_string(PROJECT_SCHEMA_KEY_SCREEN_LAYER_NAME, str);
        mReader.exit();

        if (!settings.create_layer(mIDReg, suid, str.c_str()))
            return false;
    }

    mReader.exit();

    return true;
}

ProjectSchemaSaver::~ProjectSchemaSaver()
{
    if (mWriter)
        TOMLWriter::destroy(mWriter);
}

bool ProjectSchemaSaver::save_project(Project project, std::string& toml, std::string& err)
{
    mProject = project;
    mWriter = TOMLWriter::create();
    mWriter.begin();

    mWriter.begin_table(PROJECT_SCHEMA_KEY_LUDENS_PROJECT);
    mWriter.key(PROJECT_SCHEMA_KEY_VERSION_MAJOR).write_i32(LD_VERSION_MAJOR);
    mWriter.key(PROJECT_SCHEMA_KEY_VERSION_MINOR).write_i32(LD_VERSION_MINOR);
    mWriter.key(PROJECT_SCHEMA_KEY_VERSION_PATCH).write_i32(LD_VERSION_PATCH);
    mWriter.key(PROJECT_SCHEMA_KEY_ASSETS).write_string(mProject.get_asset_schema_rel_path().string());
    mWriter.key(PROJECT_SCHEMA_KEY_NAME).write_string(mProject.get_name());
    mWriter.end_table();

    Vector<SUIDEntry> scenes;
    mProject.get_scenes(scenes);
    mWriter.begin_array_table(PROJECT_SCHEMA_KEY_SCENE);
    for (const SUIDEntry& scene : scenes)
    {
        mWriter.begin_table();
        mWriter.key(PROJECT_SCHEMA_KEY_SCENE_ID).write_u32(scene.id);
        mWriter.key(PROJECT_SCHEMA_KEY_SCENE_PATH).write_string(scene.path);
        mWriter.end_table();
    }
    mWriter.end_array_table();

    save_project_settings(project.settings(), mWriter);

    mWriter.end(toml);
    TOMLWriter::destroy(mWriter);
    mWriter = {};

    return true;
}

void ProjectSchemaSaver::save_project_settings(ProjectSettings settings, TOMLWriter writer)
{
    mWriter.begin_table(PROJECT_SCHEMA_TABLE_SETTINGS);

    writer.begin_table(PROJECT_SCHEMA_TABLE_STARTUP);
    ProjectStartupSettings startup = settings.startup_settings();
    save_project_startup_settings(startup, writer);
    writer.end_table();

    writer.begin_table(PROJECT_SCHEMA_TABLE_SCREEN_LAYER);
    ProjectScreenLayerSettings screenLayer = settings.screen_layer_settings();
    save_project_screen_layer_settings(screenLayer, writer);
    writer.end_table();

    mWriter.end_table();
}

void ProjectSchemaSaver::save_project_startup_settings(ProjectStartupSettings settings, TOMLWriter writer)
{
    writer.key(PROJECT_SCHEMA_KEY_STARTUP_WINDOW_WIDTH).write_u32(settings.get_window_width());
    writer.key(PROJECT_SCHEMA_KEY_STARTUP_WINDOW_HEIGHT).write_u32(settings.get_window_height());
    writer.key(PROJECT_SCHEMA_KEY_STARTUP_WINDOW_NAME).write_string(settings.get_window_name());
    writer.key(PROJECT_SCHEMA_KEY_DEFAULT_SCENE_ID).write_u32(settings.get_default_scene_id());
}

void ProjectSchemaSaver::save_project_screen_layer_settings(ProjectScreenLayerSettings settings, TOMLWriter writer)
{
    writer.begin_array_table(PROJECT_SCHEMA_KEY_SCREEN_LAYERS);

    for (const ProjectScreenLayer layer : settings.get_layers())
    {
        writer.begin_table();
        writer.key(PROJECT_SCHEMA_KEY_SCREEN_LAYER_ID).write_u32(layer.id);
        writer.key(PROJECT_SCHEMA_KEY_SCREEN_LAYER_NAME).write_string(layer.name);
        writer.end_table();
    }

    writer.end_array_table();
}

//
// Public API
//

bool ProjectSchema::load_project_from_source(Project project, SUIDRegistry idReg, const FS::Path& rootDir, const View& toml, std::string& err)
{
    LD_PROFILE_SCOPE;

    if (!FS::is_directory(rootDir))
    {
        err = std::format("invalid root directory [{}]", rootDir.string());
        return false;
    }

    ProjectSchemaLoader loader;
    return loader.load_project_schema(project, idReg, toml, err);
}

bool ProjectSchema::load_project_from_file(Project project, SUIDRegistry idReg, const FS::Path& tomlPath, std::string& err)
{
    LD_PROFILE_SCOPE;

    Vector<byte> toml;
    if (!FS::read_file_to_vector(tomlPath, toml, err))
        return false;

    project.set_project_schema_abs_path(FS::absolute(tomlPath));

    ProjectSchemaLoader loader;
    return loader.load_project_schema(project, idReg, View((const char*)toml.data(), toml.size()), err);
}

bool ProjectSchema::save_project_to_string(Project project, std::string& saveTOML, std::string& err)
{
    LD_PROFILE_SCOPE;

    ProjectSchemaSaver saver;
    if (!saver.save_project(project, saveTOML, err))
        return false;

    return true;
}

bool ProjectSchema::save_project(Project project, const FS::Path& savePath, std::string& err)
{
    LD_PROFILE_SCOPE;

    std::string toml;
    if (!save_project_to_string(project, toml, err))
        return false;

    View tomlView(toml.data(), toml.size());
    return FS::write_file_and_swap_backup(savePath, tomlView, err);
}

std::string ProjectSchema::create_empty(const std::string& projectName, const std::string& assetSchemaRelPath, const std::string& sceneURIPath, const std::string& sceneName)
{
    SUIDRegistry idReg = SUIDRegistry::create();
    Project project = Project::create();
    project.set_name(projectName);
    project.set_asset_schema_rel_path(assetSchemaRelPath);
    project.settings().startup_settings().set_window_name(projectName);

    std::string err;
    SUID defaultSceneID(SERIAL_TYPE_SCENE, 0);
    bool success = project.register_scene_with_id(idReg, defaultSceneID, sceneURIPath, err);
    LD_ASSERT(success);

    project.settings().startup_settings().set_default_scene_id(defaultSceneID);

    std::string toml;
    success = ProjectSchema::save_project_to_string(project, toml, err);
    LD_ASSERT(success);

    Project::destroy(project);
    SUIDRegistry::destroy(idReg);

    return toml;
}

} // namespace LD
