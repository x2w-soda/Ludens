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

[[scene]]
id = 0x03000000
path = "scenes/scene1"

[[scene]]
id = 0x03000001
path = "scenes/scene2"

[[scene]]
id = 0x03000002
path = "scenes/scene3"

[settings.startup]
window_width = 1234
window_height = 5678
window_name = 'Foo'
default_scene_id = 0x03000002
)";
    String err;
    SUIDRegistry idReg = SUIDRegistry::create();
    Project proj = Project::create();
    CHECK(ProjectSchema::load_project_from_source(proj, idReg, FS::Path("./"), View((const byte*)schemaTOML, sizeof(schemaTOML) - 1), err));

    CHECK(proj.get_name() == "hello world");

    FS::Path path = proj.get_root_dir_abs_path();
    CHECK(path.empty());

    path = proj.get_asset_schema_rel_path();
    CHECK(path == FS::Path("assets.toml"));

    CHECK(proj.get_scene_schema_rel_path(0x03000000, path));
    CHECK(path == "storage/03000000/scene.toml");
    CHECK(proj.get_scene_schema_rel_path(0x03000001, path));
    CHECK(path == "storage/03000001/scene.toml");
    CHECK(proj.get_scene_schema_rel_path(0x03000002, path));
    CHECK(path == "storage/03000002/scene.toml");
    CHECK_FALSE(proj.get_scene_schema_rel_path(0x03000003, path));

    ProjectStartupSettings startupS = proj.settings().startup_settings();
    CHECK(startupS.get_window_width() == 1234);
    CHECK(startupS.get_window_height() == 5678);
    CHECK(startupS.get_window_name() == "Foo");

    Project::destroy(proj);
    SUIDRegistry::destroy(idReg);

    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}