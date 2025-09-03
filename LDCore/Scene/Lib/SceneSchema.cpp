#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/SceneSchema.h>
#include <cstdint>

namespace LD {

static DUID load_component(TOMLValue compTOML, Scene scene);
static bool load_mesh_component(TOMLValue compTOML, Scene scene, DUID compID, const char* compName);
static void load_transform(TOMLValue transformTOML, Transform& transform);

static DUID load_component(TOMLValue compTOML, Scene scene)
{
    if (!compTOML || !scene || !compTOML.is_table_type())
        return 0;

    std::string type;
    TOMLValue typeTOML = compTOML["type"];
    if (!typeTOML || !typeTOML.is_string(type))
        return 0;

    std::string name;
    TOMLValue nameTOML = compTOML["name"];
    if (!nameTOML || !nameTOML.is_string(name))
        return 0;

    DUID compID;
    TOMLValue compIDTOML = compTOML["duid"];
    if (!compIDTOML || !compIDTOML.is_u32(compID))
        return 0;

    // TODO: table of function pointers
    if (type == "Mesh")
    {
        bool ok = load_mesh_component(compTOML, scene, compID, name.c_str());
        LD_ASSERT(ok); // TODO: deserialization error handling.
    }

    int64_t scriptID;
    TOMLValue scriptIDTOML = compTOML["script"];
    if (scriptIDTOML && scriptIDTOML.is_i64(scriptID))
    {
        // the actual script is instantiated later
        scene.create_component_script_slot(compID, (AUID)scriptID);
    }

    return compID;
}

static bool load_mesh_component(TOMLValue compTOML, Scene scene, DUID compID, const char* compName)
{
    ComponentType type;

    compID = scene.create_component(COMPONENT_TYPE_MESH, compName, (DUID)0, compID);
    if (!compID)
        return false;

    MeshComponent* meshC = (MeshComponent*)scene.get_component(compID, type);

    TOMLValue transformTOML = compTOML["transform"];
    load_transform(transformTOML, meshC->transform);

    int64_t auid;
    TOMLValue auidTOML = compTOML["auid"];
    auidTOML.is_i64(auid);
    meshC->auid = (AUID)auid;
    meshC->ruid = 0; // deferred until Scene preparation phase

    return true;
}

static void load_transform(TOMLValue transformTOML, Transform& transform)
{
    LD_ASSERT(transformTOML && transformTOML.is_table_type());

    TOMLValue positionTOML = transformTOML["position"];
    LD_ASSERT(positionTOML && positionTOML.is_array_type() && positionTOML.get_size() == 3);

    positionTOML[0].is_f32(transform.position.x);
    positionTOML[1].is_f32(transform.position.y);
    positionTOML[2].is_f32(transform.position.z);

    TOMLValue rotationTOML = transformTOML["rotation"];
    LD_ASSERT(rotationTOML && rotationTOML.is_array_type() && rotationTOML.get_size() == 3);

    rotationTOML[0].is_f32(transform.rotation.x);
    rotationTOML[1].is_f32(transform.rotation.y);
    rotationTOML[2].is_f32(transform.rotation.z);

    TOMLValue scaleTOML = transformTOML["scale"];
    LD_ASSERT(scaleTOML && scaleTOML.is_array_type() && scaleTOML.get_size() == 3);

    scaleTOML[0].is_f32(transform.scale.x);
    scaleTOML[1].is_f32(transform.scale.y);
    scaleTOML[2].is_f32(transform.scale.z);
}

void SceneSchema::load_scene(Scene scene, TOMLDocument doc)
{
    LD_PROFILE_SCOPE;

    if (!scene || !doc)
        return;

    // TODO: Scene::reset or something similar

    TOMLValue sceneTOML = doc.get("ludens_scene");
    if (!sceneTOML || sceneTOML.get_type() != TOML_TYPE_TABLE)
        return;

    int32_t version;
    TOMLValue versionTOML = sceneTOML["version"];
    if (!versionTOML || !versionTOML.is_i32(version) || version != 0)
        return;

    TOMLValue componentsTOML = doc.get("component");
    if (!componentsTOML || !componentsTOML.is_array_type())
        return;

    int32_t count = componentsTOML.get_size();
    for (int i = 0; i < count; i++)
    {
        DUID compID = load_component(componentsTOML[i], scene);
    }

    TOMLValue hierarchyTOML = doc.get("hierarchy");
    if (!hierarchyTOML || !hierarchyTOML.is_table_type())
        return;

    std::vector<std::string> keys;
    hierarchyTOML.get_keys(keys);
    for (const std::string& key : keys)
    {
        DUID parent = static_cast<DUID>(std::stoul(key));
        TOMLValue childrenTOML = hierarchyTOML[key.c_str()];
        if (!childrenTOML || !childrenTOML.is_array_type())
            continue;

        count = childrenTOML.get_size();
        for (int i = 0; i < count; i++)
        {
            DUID child;
            if (!childrenTOML[i].is_u32(child))
                continue;

            scene.reparent(child, parent);
        }
    }
}

} // namespace LD