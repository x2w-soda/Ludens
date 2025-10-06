#include <Extra/doctest/doctest.h>
#include <Ludens/Project/Project.h>
#include <Ludens/Project/ProjectSchema.h>
#include <Ludens/System/Memory.h>
#include <cstring>

using namespace LD;

TEST_CASE("ProjectSchema")
{
    const char schemaTOML[] = R"(
[ludens_project]
version_major = 0
version_minor = 0
version_patch = 0
name = "hello world"
assets = "assets.toml"
scenes = [
    "scenes/scene1.toml",
    "./scenes/scene2.toml",
    "./scenes/scene3.toml",
]
)";
    Project proj = Project::create(FS::Path("./directory"));
    ProjectSchema schema = ProjectSchema::create_from_source(schemaTOML, strlen(schemaTOML));
    CHECK(schema);

    schema.load_project(proj);

    CHECK(proj.get_name() == "hello world");

    FS::Path assetsPath = proj.get_assets_path();
    CHECK(assetsPath == FS::Path("directory/assets.toml"));

    std::vector<FS::Path> scenePaths;
    proj.get_scene_paths(scenePaths);
    CHECK(scenePaths.size() == 3);
    CHECK(scenePaths[0] == FS::Path("directory/scenes/scene1.toml"));
    CHECK(scenePaths[1] == FS::Path("directory/scenes/scene2.toml"));
    CHECK(scenePaths[2] == FS::Path("directory/scenes/scene3.toml"));

    ProjectSchema::destroy(schema);
    Project::destroy(proj);

    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}