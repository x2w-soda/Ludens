#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Version.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/SceneSchema.h>
#include <Ludens/System/Memory.h>
#include <cstdint>
#include <format>
#include <unordered_map>

namespace LD {

static CUID load_component(TOMLValue compTOML, Scene scene);
static bool load_audio_source_component(TOMLValue compTOML, Scene scene, CUID compID, const char* compName);
static bool load_camera_component(TOMLValue compTOML, Scene scene, CUID compID, const char* compName);
static bool load_mesh_component(TOMLValue compTOML, Scene scene, CUID compID, const char* compName);
static bool load_sprite2d_component(TOMLValue compTOML, Scene scene, CUID compID, const char* compName);
static void load_transform(TOMLValue transformTOML, Transform& transform);
static void load_transform_2d(TOMLValue transformTOML, Transform2D& transform);

/// @brief Scene schema implementation using TOML.
struct SceneSchemaObj
{
    TOMLDocument doc;
    std::unordered_map<CUID, TOMLValue> compValues;

    void initialize_from_doc();
};

// clang-format off
struct
{
    ComponentType type;
    bool (*load)(TOMLValue compTOML, Scene scene, CUID compID, const char* compName);
} sSceneSchemaTable[] = {
    {COMPONENT_TYPE_DATA,           nullptr},
    {COMPONENT_TYPE_AUDIO_SOURCE,   &load_audio_source_component},
    {COMPONENT_TYPE_TRANSFORM,      nullptr},
    {COMPONENT_TYPE_CAMERA,         &load_camera_component},
    {COMPONENT_TYPE_MESH,           &load_mesh_component},
    {COMPONENT_TYPE_SPRITE_2D,      &load_sprite2d_component},
};
// clang-format on

static_assert(sizeof(sSceneSchemaTable) / sizeof(*sSceneSchemaTable) == COMPONENT_TYPE_ENUM_COUNT);

static CUID load_component(TOMLValue compTOML, Scene scene)
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

    CUID compID;
    TOMLValue compIDTOML = compTOML["cuid"];
    if (!compIDTOML || !compIDTOML.is_u32(compID))
        return 0;

    // TODO: table of function pointers
    if (type == "Mesh")
    {
        bool ok = load_mesh_component(compTOML, scene, compID, name.c_str());
        LD_ASSERT(ok); // TODO: deserialization error handling.
    }
    else if (type == "AudioSource")
    {
        bool ok = load_audio_source_component(compTOML, scene, compID, name.c_str());
        LD_ASSERT(ok);
    }
    else if (type == "Sprite2D")
    {
        bool ok = load_sprite2d_component(compTOML, scene, compID, name.c_str());
        LD_ASSERT(ok);
    }
    else if (type == "Camera")
    {
        bool ok = load_camera_component(compTOML, scene, compID, name.c_str());
        LD_ASSERT(ok);
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

bool load_audio_source_component(TOMLValue compTOML, Scene scene, CUID compID, const char* compName)
{
    ComponentType type;

    compID = scene.create_component(COMPONENT_TYPE_AUDIO_SOURCE, compName, (CUID)0, compID);
    if (!compID)
        return false;

    AudioSourceComponent* sourceC = (AudioSourceComponent*)scene.get_component(compID, type);

    int64_t auid;
    TOMLValue auidTOML = compTOML["auid"];
    auidTOML.is_i64(auid);
    sourceC->clipAUID = (AUID)auid;
    sourceC->playback = {};

    return true;
}

static bool load_camera_component(TOMLValue compTOML, Scene scene, CUID compID, const char* compName)
{
    ComponentType type;

    compID = scene.create_component(COMPONENT_TYPE_CAMERA, compName, (CUID)0, compID);
    if (!compID)
        return false;

    CameraComponent* cameraC = (CameraComponent*)scene.get_component(compID, type);

    TOMLValue floatTOML{};
    TOMLValue toml = compTOML["transform"];
    load_transform(toml, cameraC->transform);
    scene.mark_component_transform_dirty(compID);

    toml = compTOML["isPerspective"];
    if (!toml || !toml.is_bool(cameraC->isPerspective))
        return false;

    toml = compTOML["isMainCamera"];
    if (!toml || !toml.is_bool(cameraC->isMainCamera))
        return false;

    if (cameraC->isPerspective)
    {
        toml = compTOML["perspective"];
        if (!toml || !toml.is_table_type())
            return false; // missing perspective info

        float fovDegrees;
        floatTOML = toml["fov"];
        if (!floatTOML || !floatTOML.is_f32(fovDegrees))
            return false;

        cameraC->perspective.fov = LD_TO_RADIANS(fovDegrees);

        floatTOML = toml["nearClip"];
        if (!floatTOML || !floatTOML.is_f32(cameraC->perspective.nearClip))
            return false;

        floatTOML = toml["farClip"];
        if (!floatTOML || !floatTOML.is_f32(cameraC->perspective.farClip))
            return false;

        // initialized later
        cameraC->perspective.aspectRatio = 0.0f;
    }
    else
    {
        toml = compTOML["orthographic"];
        if (!toml || !toml.is_table_type())
            return false; // missing orthographic info

        floatTOML = toml["left"];
        if (!floatTOML || !floatTOML.is_f32(cameraC->orthographic.left))
            return false;

        floatTOML = toml["right"];
        if (!floatTOML || !floatTOML.is_f32(cameraC->orthographic.right))
            return false;

        floatTOML = toml["bottom"];
        if (!floatTOML || !floatTOML.is_f32(cameraC->orthographic.bottom))
            return false;

        floatTOML = toml["top"];
        if (!floatTOML || !floatTOML.is_f32(cameraC->orthographic.top))
            return false;

        floatTOML = toml["nearClip"];
        if (!floatTOML || !floatTOML.is_f32(cameraC->orthographic.nearClip))
            return false;

        floatTOML = toml["farClip"];
        if (!floatTOML || !floatTOML.is_f32(cameraC->orthographic.farClip))
            return false;
    }

    return true;
}

static bool load_mesh_component(TOMLValue compTOML, Scene scene, CUID compID, const char* compName)
{
    ComponentType type;

    compID = scene.create_component(COMPONENT_TYPE_MESH, compName, (CUID)0, compID);
    if (!compID)
        return false;

    MeshComponent* meshC = (MeshComponent*)scene.get_component(compID, type);

    TOMLValue transformTOML = compTOML["transform"];
    load_transform(transformTOML, meshC->transform);
    scene.mark_component_transform_dirty(compID);

    int64_t auid;
    TOMLValue auidTOML = compTOML["auid"];
    auidTOML.is_i64(auid);
    meshC->auid = (AUID)auid;

    return true;
}

bool load_sprite2d_component(TOMLValue compTOML, Scene scene, CUID compID, const char* compName)
{
    ComponentType type;

    compID = scene.create_component(COMPONENT_TYPE_SPRITE_2D, compName, (CUID)0, compID);
    if (!compID)
        return false;

    Sprite2DComponent* spriteC = (Sprite2DComponent*)scene.get_component(compID, type);

    TOMLValue transformTOML = compTOML["transform"];
    load_transform_2d(transformTOML, spriteC->transform);
    scene.mark_component_transform_dirty(compID);

    int64_t auid;
    TOMLValue auidTOML = compTOML["auid"];
    auidTOML.is_i64(auid);
    spriteC->auid = (AUID)auid;

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
    transform.quat = Quat::from_euler(transform.rotation);

    TOMLValue scaleTOML = transformTOML["scale"];
    LD_ASSERT(scaleTOML && scaleTOML.is_array_type() && scaleTOML.get_size() == 3);

    scaleTOML[0].is_f32(transform.scale.x);
    scaleTOML[1].is_f32(transform.scale.y);
    scaleTOML[2].is_f32(transform.scale.z);
}

void load_transform_2d(TOMLValue transformTOML, Transform2D& transform)
{
    LD_ASSERT(transformTOML && transformTOML.is_table_type());

    TOMLValue positionTOML = transformTOML["position"];
    LD_ASSERT(positionTOML && positionTOML.is_array_type() && positionTOML.get_size() == 2);
    positionTOML[0].is_f32(transform.position.x);
    positionTOML[1].is_f32(transform.position.y);

    TOMLValue rotationTOML = transformTOML["rotation"];
    LD_ASSERT(rotationTOML && rotationTOML.is_float_type());
    rotationTOML.is_f32(transform.rotation);

    TOMLValue scaleTOML = transformTOML["scale"];
    LD_ASSERT(scaleTOML && scaleTOML.is_array_type() && scaleTOML.get_size() == 2);
    scaleTOML[0].is_f32(transform.scale.x);
    scaleTOML[1].is_f32(transform.scale.y);
}

void SceneSchemaObj::initialize_from_doc()
{
    compValues.clear();

    TOMLValue componentsTOML = doc.get("component");
    if (componentsTOML && componentsTOML.is_array_type())
    {
        // extract component tables
        int count = componentsTOML.get_size();
        for (int i = 0; i < count; i++)
        {
            TOMLValue compTOML = componentsTOML[i];
            if (!compTOML.is_table_type())
                continue;

            CUID compID;
            TOMLValue compIDTOML = compTOML["cuid"];
            if (compIDTOML && compIDTOML.is_u32(compID))
                compValues[compID] = compTOML;
        }
    }
}

void SceneSchema::load_scene(Scene scene)
{
    LD_PROFILE_SCOPE;

    TOMLDocument doc = mObj->doc;

    if (!scene || !doc)
        return;

    TOMLValue sceneTOML = doc.get("ludens_scene");
    if (!sceneTOML || sceneTOML.get_type() != TOML_TYPE_TABLE)
        return;

    int32_t version;
    TOMLValue versionTOML = sceneTOML["version_major"];
    if (!versionTOML || !versionTOML.is_i32(version) || version != LD_VERSION_MAJOR)
        return;

    versionTOML = sceneTOML["version_minor"];
    if (!versionTOML || !versionTOML.is_i32(version) || version != LD_VERSION_MINOR)
        return;

    versionTOML = sceneTOML["version_patch"];
    if (!versionTOML || !versionTOML.is_i32(version) || version != LD_VERSION_PATCH)
        return;

    for (auto ite : mObj->compValues)
    {
        TOMLValue compTOML = ite.second;
        CUID compID = load_component(compTOML, scene);
        LD_ASSERT(compID == ite.first); // TODO: error handling
    }

    TOMLValue hierarchyTOML = doc.get("hierarchy");
    if (!hierarchyTOML || !hierarchyTOML.is_table_type())
        return;

    std::vector<std::string> keys;
    hierarchyTOML.get_keys(keys);
    for (const std::string& key : keys)
    {
        CUID parent = static_cast<CUID>(std::stoul(key));
        TOMLValue childrenTOML = hierarchyTOML[key.c_str()];
        if (!childrenTOML || !childrenTOML.is_array_type())
            continue;

        int count = childrenTOML.get_size();
        for (int i = 0; i < count; i++)
        {
            CUID child;
            if (!childrenTOML[i].is_u32(child))
                continue;

            scene.reparent(child, parent);
        }
    }
}

bool SceneSchema::save_to_disk(const FS::Path& savePath)
{
    return mObj->doc.save_to_disk(savePath);
}

AUID SceneSchema::get_component_script(CUID compID)
{
    auto ite = mObj->compValues.find(compID);

    if (ite == mObj->compValues.end())
        return 0;

    TOMLValue compTOML = ite->second;
    TOMLValue scriptIDTOML = compTOML["script"];

    int64_t scriptAssetID;
    if (!scriptIDTOML || !scriptIDTOML.is_i64(scriptAssetID))
        return 0;

    return (AUID)scriptAssetID;
}

void SceneSchema::set_component_script(CUID compID, AUID scriptAssetID)
{
    auto ite = mObj->compValues.find(compID);

    if (ite == mObj->compValues.end())
        return;

    TOMLValue compTOML = ite->second;
    TOMLValue scriptTOML;
    TOMLType valueType = TOML_TYPE_INT;

    if (!compTOML.has_key("script", &valueType))
        scriptTOML = compTOML.set_key("script", valueType);
    else
        scriptTOML = compTOML.get_key("script");

    LD_ASSERT(scriptTOML);
    scriptTOML.set_i64((int64_t)scriptAssetID);
}

SceneSchema SceneSchema::create_from_source(const char* source, size_t len)
{
    SceneSchemaObj* obj = heap_new<SceneSchemaObj>(MEMORY_USAGE_SCHEMA);

    std::string err;
    obj->doc = TOMLDocument::create();
    if (!obj->doc.parse(source, len, err))
    {
        heap_delete<SceneSchemaObj>(obj);
        return {};
    }

    obj->initialize_from_doc();

    return SceneSchema(obj);
}

SceneSchema SceneSchema::create_from_file(const FS::Path& tomlPath)
{
    SceneSchemaObj* obj = heap_new<SceneSchemaObj>(MEMORY_USAGE_SCHEMA);

    obj->doc = TOMLDocument::create_from_file(tomlPath);
    if (!obj->doc)
    {
        heap_delete<SceneSchemaObj>(obj);
        return {};
    }

    obj->initialize_from_doc();

    return SceneSchema(obj);
}

void SceneSchema::destroy(SceneSchema schema)
{
    SceneSchemaObj* obj = schema.unwrap();

    TOMLDocument::destroy(obj->doc);

    heap_delete<SceneSchemaObj>(obj);
}

std::string SceneSchema::get_default_text()
{
    return std::format(R"(
[ludens_scene]
version_major = {}
version_minor = {}
version_patch = {}
)",
                       LD_VERSION_MAJOR,
                       LD_VERSION_MINOR,
                       LD_VERSION_PATCH);
}

} // namespace LD
