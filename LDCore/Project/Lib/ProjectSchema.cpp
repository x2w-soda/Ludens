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
    void load_project_settings(ProjectSettings settings, TOMLValue settingsTOML);
    void load_project_startup_settings(ProjectStartupSettings settings, TOMLValue startupTOML);
    void load_project_screen_layer_settings(ProjectScreenLayerSettings settings, TOMLValue screenLayerTOML);

private:
    Project mProject{};
    TOMLDocument mDoc{};
};

ProjectSchemaLoader::~ProjectSchemaLoader()
{
    if (mDoc)
        TOMLDocument::destroy(mDoc);
}

bool ProjectSchemaLoader::load_project(Project project, const View& toml, std::string& err)
{
    mDoc = TOMLDocument::create();

    if (!TOMLParser::parse(mDoc, toml, err))
        return false;

    TOMLValue projectTOML = mDoc.get(PROJECT_SCHEMA_KEY_LUDENS_PROJECT);
    if (!projectTOML || !projectTOML.is_table())
        return false;

    int32_t versionMajor;
    TOMLValue versionMajorTOML = projectTOML[PROJECT_SCHEMA_KEY_VERSION_MAJOR];
    if (!versionMajorTOML || !versionMajorTOML.get_i32(versionMajor) || versionMajor != LD_VERSION_MAJOR)
        return false;

    int32_t versionMinor;
    TOMLValue versionMinorTOML = projectTOML[PROJECT_SCHEMA_KEY_VERSION_MINOR];
    if (!versionMinorTOML || !versionMinorTOML.get_i32(versionMinor) || versionMinor != LD_VERSION_MINOR)
        return false;

    int32_t versionPatch;
    TOMLValue versionPatchTOML = projectTOML[PROJECT_SCHEMA_KEY_VERSION_PATCH];
    if (!versionPatchTOML || !versionPatchTOML.get_i32(versionPatch) || versionPatch != LD_VERSION_PATCH)
        return false;

    std::string str;
    TOMLValue nameTOML = projectTOML[PROJECT_SCHEMA_KEY_NAME];
    if (!nameTOML || !nameTOML.get_string(str))
        return false;

    project.set_name(str);

    TOMLValue assetsTOML = projectTOML[PROJECT_SCHEMA_KEY_ASSETS];
    if (!assetsTOML || !assetsTOML.get_string(str))
        return false;

    project.set_assets_path(FS::Path(str));

    TOMLValue scenesTOML = projectTOML[PROJECT_SCHEMA_KEY_SCENES];
    if (!scenesTOML || !scenesTOML.is_array())
        return false;

    int sceneCount = scenesTOML.size();
    for (int i = 0; i < sceneCount; i++)
    {
        TOMLValue sceneTOML = scenesTOML[i];
        if (sceneTOML.get_string(str))
            project.add_scene_path(FS::Path(str));
    }

    TOMLValue settingsTOML = mDoc.get(PROJECT_SCHEMA_TABLE_SETTINGS);
    if (!settingsTOML || !settingsTOML.is_table())
        return false;

    load_project_settings(project.get_settings(), settingsTOML);

    TOMLDocument::destroy(mDoc);
    mDoc = {};

    return true;
}

void ProjectSchemaLoader::load_project_settings(ProjectSettings settings, TOMLValue settingsTOML)
{
    TOMLValue startupTOML = settingsTOML[PROJECT_SCHEMA_TABLE_STARTUP];

    if (startupTOML)
    {
        ProjectStartupSettings startup = settings.get_startup_settings();
        load_project_startup_settings(startup, startupTOML);
    }

    TOMLValue screenLayerTOML = settingsTOML[PROJECT_SCHEMA_TABLE_SCREEN_LAYER];

    if (screenLayerTOML)
    {
        ProjectScreenLayerSettings screenLayer = settings.get_screen_layer_settings();
        load_project_screen_layer_settings(screenLayer, startupTOML);
    }
}

void ProjectSchemaLoader::load_project_startup_settings(ProjectStartupSettings settings, TOMLValue startupTOML)
{
    uint32_t windowWidth = DEFAULT_STARTUP_WINDOW_WIDTH;
    TOMLValue windowWidthTOML = startupTOML[PROJECT_SCHEMA_KEY_STARTUP_WINDOW_WIDTH];
    if (windowWidthTOML)
        windowWidthTOML.get_u32(windowWidth);

    settings.set_window_width(windowWidth);

    uint32_t windowHeight = DEFAULT_STARTUP_WINDOW_HEIGHT;
    TOMLValue windowHeightTOML = startupTOML[PROJECT_SCHEMA_KEY_STARTUP_WINDOW_HEIGHT];
    if (windowHeightTOML)
        windowHeightTOML.get_u32(windowHeight);

    settings.set_window_height(windowHeight);

    std::string windowName = DEFAULT_STARTUP_WINDOW_NAME;
    TOMLValue windowNameTOML = startupTOML[PROJECT_SCHEMA_KEY_STARTUP_WINDOW_NAME];
    if (windowNameTOML)
        windowNameTOML.get_string(windowName);

    settings.set_window_name(windowName);

    std::string defaultScenePath = DEFAULT_STARTUP_DEFAULT_SCENE_PATH;
    TOMLValue defaultScenePathTOML = startupTOML[PROJECT_SCHEMA_KEY_DEFAULT_SCENE_PATH];
    if (defaultScenePathTOML)
        defaultScenePathTOML.get_string(defaultScenePath);

    settings.set_default_scene_path(defaultScenePath);
}

void ProjectSchemaLoader::load_project_screen_layer_settings(ProjectScreenLayerSettings settings, TOMLValue screenLayerTOML)
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
    mWriter = TOMLWriter::create();
    mWriter.begin();

    mWriter.begin_table(PROJECT_SCHEMA_KEY_LUDENS_PROJECT);
    mWriter.key(PROJECT_SCHEMA_KEY_VERSION_MAJOR).value_i32(LD_VERSION_MAJOR);
    mWriter.key(PROJECT_SCHEMA_KEY_VERSION_MINOR).value_i32(LD_VERSION_MINOR);
    mWriter.key(PROJECT_SCHEMA_KEY_VERSION_PATCH).value_i32(LD_VERSION_PATCH);
    mWriter.end_table();

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
    writer.key(PROJECT_SCHEMA_KEY_STARTUP_WINDOW_WIDTH).value_u32(settings.get_window_width());
    writer.key(PROJECT_SCHEMA_KEY_STARTUP_WINDOW_HEIGHT).value_u32(settings.get_window_height());
    writer.key(PROJECT_SCHEMA_KEY_STARTUP_WINDOW_NAME).value_string(settings.get_window_name());
    writer.key(PROJECT_SCHEMA_KEY_DEFAULT_SCENE_PATH).value_string(settings.get_default_scene_path());
}

void ProjectSchemaSaver::save_project_screen_layer_settings(ProjectScreenLayerSettings settings, TOMLWriter writer)
{
    // TODO:
}

//
// Public API
//

bool ProjectSchema::load_project_from_source(Project project, const View& toml, std::string& err)
{
    LD_PROFILE_SCOPE;

    ProjectSchemaLoader loader;
    if (!loader.load_project(project, toml, err))
        return false;

    return true;
}

bool ProjectSchema::load_project_from_file(Project project, const FS::Path& tomlPath, std::string& err)
{
    LD_PROFILE_SCOPE;

    Vector<byte> toml;
    if (!FS::read_file_to_vector(tomlPath, toml))
        return false;

    View tomlView((const char*)toml.data(), toml.size());
    return load_project_from_source(project, tomlView, err);
}

bool ProjectSchema::save_project(Project project, const FS::Path& savePath, std::string& err)
{
    LD_PROFILE_SCOPE;

    std::string toml;
    ProjectSchemaSaver saver;
    if (!saver.save_project(project, toml, err))
        return false;

    return FS::write_file_and_swap_backup(savePath, toml.size(), (const byte*)toml.data(), err);
}

} // namespace LD
