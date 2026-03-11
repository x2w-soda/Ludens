#include <Extra/doctest/doctest.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Project/Project.h>
#include <Ludens/Project/ProjectSchema.h>
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

[settings.startup]
window_width = 1234
window_height = 5678
window_name = 'Foo'
)";
    std::string err;
    Project proj = Project::create();
    CHECK(ProjectSchema::load_project_from_source(proj, FS::Path("./"), View(schemaTOML, sizeof(schemaTOML) - 1), err));

    CHECK(proj.get_name() == "hello world");

    FS::Path path = proj.get_root_path();
    CHECK(path.empty());

    path = proj.get_asset_schema_path();
    CHECK(path == FS::Path("assets.toml"));

    path = proj.get_asset_schema_absolute_path();
    CHECK(path == FS::Path("assets.toml"));

    Vector<FS::Path> scenePaths;
    proj.get_scene_absolute_paths(scenePaths);
    CHECK(scenePaths.size() == 3);
    CHECK(scenePaths[0] == FS::Path("scenes/scene1.toml"));
    CHECK(scenePaths[1] == FS::Path("scenes/scene2.toml"));
    CHECK(scenePaths[2] == FS::Path("scenes/scene3.toml"));

    ProjectStartupSettings startupS = proj.get_settings().get_startup_settings();
    CHECK(startupS.get_window_width() == 1234);
    CHECK(startupS.get_window_height() == 5678);
    CHECK(startupS.get_window_name() == "Foo");

    Project::destroy(proj);

    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}