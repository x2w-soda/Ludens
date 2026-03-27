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

    bool load_project(Project project, const View& toml, std::string& err);

private:
    bool load_project_settings(ProjectSettings settings, std::string& err);
    void load_project_startup_settings(ProjectStartupSettings settings);
    void load_project_screen_layer_settings(ProjectScreenLayerSettings settings);

private:
    Project mProject{};
    TOMLReader mReader{};
};

ProjectSchemaLoader::~ProjectSchemaLoader()
{
    if (mReader)
        TOMLReader::destroy(mReader);
}

bool ProjectSchemaLoader::load_project(Project project, const View& toml, std::string& err)
{
    mReader = TOMLReader::create(toml, err);
    mProject = project;

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

    project.set_asset_schema_path(FS::Path(str));

    bool isValid = true;
    uint32_t u32;
    ProjectSceneEntry sceneEntry;
    int sceneCount = 0;
    if (mReader.enter_array(PROJECT_SCHEMA_KEY_SCENE, sceneCount))
    {
        for (int i = 0; isValid && i < sceneCount; i++)
        {
            if (!mReader.enter_table(i))
                continue; // ignore

            if (!mReader.read_string(PROJECT_SCHEMA_KEY_SCENE_NAME, sceneEntry.name))
            {
                mReader.exit();
                continue;
            }

            if (!mReader.read_string(PROJECT_SCHEMA_KEY_SCENE_PATH, str))
            {
                mReader.exit();
                continue;
            }

            sceneEntry.path = str;

            if (!mReader.read_u32(PROJECT_SCHEMA_KEY_SCENE_ID, u32))
            {
                mReader.exit();
                continue;
            }

            sceneEntry.id = SUID(u32);

            // conflicting scene entry makes the entire project schema invalid
            if (!project.add_scene(sceneEntry, err))
                isValid = false;

            mReader.exit();
        }
        mReader.exit();
    }

    if (!isValid)
        return false;

    if (mReader.enter_table(PROJECT_SCHEMA_TABLE_SETTINGS))
    {
        isValid = load_project_settings(project.get_settings(), err);
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
        ProjectStartupSettings startup = settings.get_startup_settings();
        load_project_startup_settings(startup);
        mReader.exit();

        ProjectSceneEntry dftScene;
        SUID dftSceneID = startup.get_default_scene_id();
        if (!mProject.get_scene(dftSceneID, dftScene))
        {
            err = std::format("project startup settings: default_scene_id {} does not exist", dftSceneID);
            return false;
        }
    }

    if (mReader.enter_table(PROJECT_SCHEMA_TABLE_SCREEN_LAYER))
    {
        ProjectScreenLayerSettings screenLayer = settings.get_screen_layer_settings();
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

void ProjectSchemaLoader::load_project_screen_layer_settings(ProjectScreenLayerSettings settings)
{
    // TODO:
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
    mWriter.key(PROJECT_SCHEMA_KEY_ASSETS).write_string(mProject.get_asset_schema_path().string());
    mWriter.key(PROJECT_SCHEMA_KEY_NAME).write_string(mProject.get_name());
    mWriter.end_table();

    Vector<ProjectSceneEntry> scenes;
    mProject.get_scenes(scenes);
    mWriter.begin_array_table(PROJECT_SCHEMA_KEY_SCENE);
    for (const ProjectSceneEntry& scene : scenes)
    {
        mWriter.begin_table();
        mWriter.key(PROJECT_SCHEMA_KEY_SCENE_ID).write_u32(scene.id);
        mWriter.key(PROJECT_SCHEMA_KEY_SCENE_NAME).write_string(scene.name);
        mWriter.key(PROJECT_SCHEMA_KEY_SCENE_PATH).write_string(scene.path.string());
        mWriter.end_table();
    }
    mWriter.end_array_table();

    save_project_settings(project.get_settings(), mWriter);

    mWriter.end(toml);
    TOMLWriter::destroy(mWriter);
    mWriter = {};

    return true;
}

void ProjectSchemaSaver::save_project_settings(ProjectSettings settings, TOMLWriter writer)
{
    mWriter.begin_table(PROJECT_SCHEMA_TABLE_SETTINGS);

    writer.begin_table(PROJECT_SCHEMA_TABLE_STARTUP);
    ProjectStartupSettings startup = settings.get_startup_settings();
    save_project_startup_settings(startup, writer);
    writer.end_table();

    writer.begin_table(PROJECT_SCHEMA_TABLE_SCREEN_LAYER);
    ProjectScreenLayerSettings screenLayer = settings.get_screen_layer_settings();
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
    // TODO:
}

//
// Public API
//

bool ProjectSchema::load_project_from_source(Project project, const FS::Path& rootDir, const View& toml, std::string& err)
{
    LD_PROFILE_SCOPE;

    if (!FS::is_directory(rootDir))
    {
        err = std::format("invalid root directory [{}]", rootDir.string());
        return false;
    }

    project.set_project_schema_path(rootDir / FS::Path("project.toml"));

    ProjectSchemaLoader loader;
    return loader.load_project(project, toml, err);
}

bool ProjectSchema::load_project_from_file(Project project, const FS::Path& tomlPath, std::string& err)
{
    LD_PROFILE_SCOPE;

    Vector<byte> toml;
    if (!FS::read_file_to_vector(tomlPath, toml, err))
        return false;

    project.set_project_schema_path(tomlPath);

    ProjectSchemaLoader loader;
    return loader.load_project(project, View((const char*)toml.data(), toml.size()), err);
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

std::string ProjectSchema::create_empty(const std::string& projectName, const std::string& assetSchemaPath)
{
    Project project = Project::create();
    project.set_name(projectName);
    project.set_asset_schema_path(assetSchemaPath);
    project.get_settings().get_startup_settings().set_window_name(projectName);

    std::string toml, err;
    bool success = ProjectSchema::save_project_to_string(project, toml, err);
    LD_ASSERT(success);

    Project::destroy(project);

    return toml;
}

} // namespace LD
