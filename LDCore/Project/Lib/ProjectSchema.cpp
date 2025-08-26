#include <Ludens/Header/Version.h>
#include <Ludens/Project/ProjectSchema.h>

namespace fs = std::filesystem;

namespace LD {

void ProjectSchema::load_project(Project project, TOMLDocument doc)
{
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

    project.set_assets_path(fs::path(str));

    TOMLValue scenesTOML = projectTOML["scenes"];
    if (!scenesTOML || !scenesTOML.is_array_type())
        return;

    int sceneCount = scenesTOML.get_size();
    for (int i = 0; i < sceneCount; i++)
    {
        TOMLValue sceneTOML = scenesTOML[i];
        if (sceneTOML.is_string(str))
            project.add_scene_path(fs::path(str));
    }
}

} // namespace LD