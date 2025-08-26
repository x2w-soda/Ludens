#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/SceneSchema.h>
#include <cstdint>

namespace LD {

static DUID load_component(TOMLValue compTOML, Scene scene);
static DUID load_mesh_component(TOMLValue compTOML, Scene scene, const char* compName);
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

    DUID compID = 0;

    // TODO: table of function pointers
    if (type == "Mesh")
    {
        compID = load_mesh_component(compTOML, scene, name.c_str());
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

static DUID load_mesh_component(TOMLValue compTOML, Scene scene, const char* compName)
{
    ComponentType type;
    DUID compID = scene.create_component(COMPONENT_TYPE_MESH, compName, (DUID)0);
    MeshComponent* meshC = (MeshComponent*)scene.get_component(compID, type);

    TOMLValue transformTOML = compTOML["transform"];
    load_transform(transformTOML, meshC->transform);

    int64_t auid;
    TOMLValue auidTOML = compTOML["auid"];
    auidTOML.is_i64(auid);
    meshC->auid = (AUID)auid;
    meshC->ruid = 0; // deferred until Scene preparation phase

    return compID;
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

    TOMLValue tomlScene = doc.get("ludens_scene");
    if (!tomlScene || tomlScene.get_type() != TOML_TYPE_TABLE)
        return;

    int32_t version;
    TOMLValue tomlVal = tomlScene.get_key("version");
    if (!tomlVal || !tomlVal.is_i32(version) || version != 0)
        return;

    TOMLValue tomlComponents = doc.get("component");
    if (!tomlComponents || !tomlComponents.is_array_type())
        return;

    int32_t componentCount = tomlComponents.get_size();
    for (int i = 0; i < componentCount; i++)
    {
        DUID compID = load_component(tomlComponents.get_index(i), scene);
    }
}

} // namespace LD