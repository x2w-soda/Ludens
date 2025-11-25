#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Version.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Project/ProjectSchema.h>
#include <Ludens/System/Memory.h>
#include <format>

#include "ProjectSettingsDefault.h"

namespace LD {

static void load_project_from_schema(Project project, TOMLDocument doc);
static void load_project_settings(ProjectSettings settings, TOMLValue settingsTOML);
static void load_project_startup_settings(ProjectStartupSettings settings, TOMLValue startupTOML);

static void load_project_from_schema(Project project, TOMLDocument doc)
{
    TOMLValue projectTOML = doc.get("ludens_project");
    if (!projectTOML || !projectTOML.is_table_type())
        return;

    int32_t versionMajor;
    TOMLValue versionMajorTOML = projectTOML["version_major"];
    if (!versionMajorTOML || !versionMajorTOML.get_i32(versionMajor) || versionMajor != LD_VERSION_MAJOR)
        return;

    int32_t versionMinor;
    TOMLValue versionMinorTOML = projectTOML["version_minor"];
    if (!versionMinorTOML || !versionMinorTOML.get_i32(versionMinor) || versionMinor != LD_VERSION_MINOR)
        return;

    int32_t versionPatch;
    TOMLValue versionPatchTOML = projectTOML["version_patch"];
    if (!versionPatchTOML || !versionPatchTOML.get_i32(versionPatch) || versionPatch != LD_VERSION_PATCH)
        return;

    std::string str;
    TOMLValue nameTOML = projectTOML["name"];
    if (!nameTOML || !nameTOML.get_string(str))
        return;

    project.set_name(str);

    TOMLValue assetsTOML = projectTOML["assets"];
    if (!assetsTOML || !assetsTOML.get_string(str))
        return;

    project.set_assets_path(FS::Path(str));

    TOMLValue scenesTOML = projectTOML["scenes"];
    if (!scenesTOML || !scenesTOML.is_array_type())
        return;

    int sceneCount = scenesTOML.get_size();
    for (int i = 0; i < sceneCount; i++)
    {
        TOMLValue sceneTOML = scenesTOML[i];
        if (sceneTOML.get_string(str))
            project.add_scene_path(FS::Path(str));
    }

    TOMLValue settingsTOML = doc.get("settings");
    if (!settingsTOML || !settingsTOML.is_table_type())
        return;

    load_project_settings(project.get_settings(), settingsTOML);
}

static void load_project_settings(ProjectSettings settings, TOMLValue settingsTOML)
{
    TOMLValue startupTOML = settingsTOML["startup"];

    if (startupTOML)
    {
        ProjectStartupSettings startup = settings.get_startup_settings();
        load_project_startup_settings(startup, startupTOML);
    }
}

static void load_project_startup_settings(ProjectStartupSettings settings, TOMLValue startupTOML)
{
    uint32_t windowWidth = DEFAULT_STARTUP_WINDOW_WIDTH;
    TOMLValue windowWidthTOML = startupTOML["window_width"];
    if (windowWidthTOML)
        windowWidthTOML.get_u32(windowWidth);

    settings.set_window_width(windowWidth);

    uint32_t windowHeight = DEFAULT_STARTUP_WINDOW_HEIGHT;
    TOMLValue windowHeightTOML = startupTOML["window_height"];
    if (windowHeightTOML)
        windowHeightTOML.get_u32(windowHeight);

    settings.set_window_height(windowHeight);

    std::string windowName = DEFAULT_STARTUP_WINDOW_NAME;
    TOMLValue windowNameTOML = startupTOML["window_name"];
    if (windowNameTOML)
        windowNameTOML.get_string(windowName);

    settings.set_window_name(windowName);
}

void ProjectSchema::load_project_from_source(Project project, const char* source, size_t len)
{
    TOMLDocument doc = TOMLDocument::create();

    std::string err;
    bool success = doc.parse(source, len, err);
    if (!success)
    {
        TOMLDocument::destroy(doc);
        return; // TODO: error handling path
    }

    load_project_from_schema(project, doc);
    TOMLDocument::destroy(doc);
}

void ProjectSchema::load_project_from_file(Project project, const FS::Path& tomlPath)
{
    TOMLDocument doc = TOMLDocument::create_from_file(tomlPath);
    load_project_from_schema(project, doc);
    TOMLDocument::destroy(doc);
}

std::string ProjectSchema::get_default_text(const std::string& projectName, const FS::Path& assetSchemaPath)
{
    return std::format(R"(
[ludens_project]
version_major = {}
version_minor = {}
version_patch = {}
name = {}
assets = {}
scenes = []

[settings.startup]
window_width = {}
window_height = {}
window_name = '{}'
)",
                       LD_VERSION_MAJOR,
                       LD_VERSION_MINOR,
                       LD_VERSION_PATCH,
                       projectName,
                       assetSchemaPath.string(),
                       DEFAULT_STARTUP_WINDOW_WIDTH,
                       DEFAULT_STARTUP_WINDOW_HEIGHT,
                       DEFAULT_STARTUP_WINDOW_NAME);
}

} // namespace LD
