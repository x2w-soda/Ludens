#include <Ludens/Header/Version.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Project/ProjectSchema.h>
#include <Ludens/System/Memory.h>
#include <format>

namespace LD {

/// @brief Project schema implementation via TOML.
struct ProjectSchemaObj
{
    TOMLDocument doc;
};

ProjectSchema ProjectSchema::create_from_source(const char* source, size_t len)
{
    ProjectSchemaObj* obj = heap_new<ProjectSchemaObj>(MEMORY_USAGE_SCHEMA);

    std::string err;
    obj->doc = TOMLDocument::create();
    if (!obj->doc.parse(source, len, err))
    {
        heap_delete<ProjectSchemaObj>(obj);
        return {};
    }

    return ProjectSchema(obj);
}

ProjectSchema ProjectSchema::create_from_file(const FS::Path& tomlPath)
{
    ProjectSchemaObj* obj = heap_new<ProjectSchemaObj>(MEMORY_USAGE_SCHEMA);

    obj->doc = TOMLDocument::create_from_file(tomlPath);
    if (!obj->doc)
    {
        heap_delete<ProjectSchemaObj>(obj);
        return {};
    }

    return ProjectSchema(obj);
}

void ProjectSchema::destroy(ProjectSchema schema)
{
    ProjectSchemaObj* obj = schema.unwrap();

    TOMLDocument::destroy(obj->doc);

    heap_delete<ProjectSchemaObj>(obj);
}

std::string ProjectSchema::get_default_text(const std::string& projectName, const FS::Path& assetSchemaPath)
{
    return std::format(R"(
[ludens_scene]
version_major = {}
version_minor = {}
version_patch = {}
name = {}
assets = {}
scenes = []
)",
                       LD_VERSION_MAJOR,
                       LD_VERSION_MINOR,
                       LD_VERSION_PATCH,
                       projectName,
                       assetSchemaPath.string());
}

void ProjectSchema::load_project(Project project)
{
    TOMLDocument doc = mObj->doc;

    TOMLValue projectTOML = doc.get("ludens_project");
    if (!projectTOML || !projectTOML.is_table_type())
        return;

    int32_t versionMajor;
    TOMLValue versionMajorTOML = projectTOML["version_major"];
    if (!versionMajorTOML || !versionMajorTOML.is_i32(versionMajor) || versionMajor != LD_VERSION_MAJOR)
        return;

    int32_t versionMinor;
    TOMLValue versionMinorTOML = projectTOML["version_minor"];
    if (!versionMinorTOML || !versionMinorTOML.is_i32(versionMinor) || versionMinor != LD_VERSION_MINOR)
        return;

    int32_t versionPatch;
    TOMLValue versionPatchTOML = projectTOML["version_patch"];
    if (!versionPatchTOML || !versionPatchTOML.is_i32(versionPatch) || versionPatch != LD_VERSION_PATCH)
        return;

    std::string str;
    TOMLValue nameTOML = projectTOML["name"];
    if (!nameTOML || !nameTOML.is_string(str))
        return;

    project.set_name(str);

    TOMLValue assetsTOML = projectTOML["assets"];
    if (!assetsTOML || !assetsTOML.is_string(str))
        return;

    project.set_assets_path(FS::Path(str));

    TOMLValue scenesTOML = projectTOML["scenes"];
    if (!scenesTOML || !scenesTOML.is_array_type())
        return;

    int sceneCount = scenesTOML.get_size();
    for (int i = 0; i < sceneCount; i++)
    {
        TOMLValue sceneTOML = scenesTOML[i];
        if (sceneTOML.is_string(str))
            project.add_scene_path(FS::Path(str));
    }
}

} // namespace LD
