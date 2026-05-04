#include <Ludens/DSA/HashMap.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/DSA/ViewUtil.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Version.h>
#include <Ludens/Media/Format/TOML.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/ComponentViews.h>
#include <Ludens/Scene/SceneSchema.h>

#include <cstdint>
#include <format>

#include "SceneSchemaKeys.h"

namespace LD {

/// @brief Saves Scene to TOML schema.
class SceneSchemaSaver
{
public:
    SceneSchemaSaver() = default;
    SceneSchemaSaver(const SceneSchemaSaver&) = delete;
    ~SceneSchemaSaver();

    SceneSchemaSaver& operator=(const SceneSchemaSaver&) = delete;

    bool save_scene(Scene scene, std::string& toml, std::string& err);

    static bool save_audio_source(SceneSchemaSaver& saver, ComponentView comp);
    static bool save_transform_2d(SceneSchemaSaver& saver, ComponentView comp);
    static bool save_camera(SceneSchemaSaver& saver, ComponentView comp);
    static bool save_camera_2d(SceneSchemaSaver& saver, ComponentView comp);
    // static bool save_mesh(SceneSchemaSaver& saver, ComponentView comp);
    static bool save_sprite_2d(SceneSchemaSaver& saver, ComponentView comp);
    // static bool save_screen_ui(SceneSchemaSaver& saver, ComponentView comp);

private:
    static void save(SceneSchemaSaver& saver, ComponentView comp);

private:
    Scene mScene{};
    HashMap<SUID, Vector<SUID>> mChildMap;
    TOMLWriter mWriter{};
};

/// @brief Loads Scene from TOML schema.
class SceneSchemaLoader
{
public:
    SceneSchemaLoader() = default;
    SceneSchemaLoader(const SceneSchemaLoader&) = delete;
    ~SceneSchemaLoader();

    SceneSchemaLoader& operator=(const SceneSchemaLoader&) = delete;

    bool load_scene(Scene scene, SUIDRegistry idReg, const View& toml, std::string& err);

    static ComponentView load_audio_source(SceneSchemaLoader& loader, SUID compSUID, const char* compName);
    static ComponentView load_transform_2d(SceneSchemaLoader& loader, SUID compSUID, const char* compName);
    static ComponentView load_camera(SceneSchemaLoader& loader, SUID compSUID, const char* compName);
    static ComponentView load_camera_2d(SceneSchemaLoader& loader, SUID compSUID, const char* compName);
    // static ComponentView load_mesh(SceneSchemaLoader& loader, SUID compSUID, const char* compName);
    static ComponentView load_sprite_2d(SceneSchemaLoader& loader, SUID compSUID, const char* compName);
    // static ComponentView load_screen_ui(SceneSchemaLoader& loader, SUID compSUID, const char* compName);

private:
    static ComponentView load(SceneSchemaLoader& loader, std::string& err);

private:
    Scene mScene{};
    TOMLReader mReader{};
    SUIDRegistry mIDReg{};
};

// clang-format off
struct
{
    ComponentType type;
    const char* compTypeName;
    ComponentView (*load)(SceneSchemaLoader& loader, SUID compSUID, const char* compName);
    bool (*save)(SceneSchemaSaver& saver, ComponentView comp);
} sSceneSchemaTable[] = {
    {COMPONENT_TYPE_DATA,           "Data",        nullptr,                                           nullptr},
    {COMPONENT_TYPE_AUDIO_SOURCE,   "AudioSource", &SceneSchemaLoader::load_audio_source,   &SceneSchemaSaver::save_audio_source},
    {COMPONENT_TYPE_TRANSFORM,      "Transform",   nullptr,                                           nullptr},
    {COMPONENT_TYPE_TRANSFORM_2D,   "Transform2D", &SceneSchemaLoader::load_transform_2d,   &SceneSchemaSaver::save_transform_2d},
    {COMPONENT_TYPE_CAMERA,         "Camera",      &SceneSchemaLoader::load_camera,         &SceneSchemaSaver::save_camera},
    {COMPONENT_TYPE_CAMERA_2D,      "Camera2D",    &SceneSchemaLoader::load_camera_2d,      &SceneSchemaSaver::save_camera_2d},
    {COMPONENT_TYPE_MESH,           "Mesh",        nullptr,                                           nullptr},
    {COMPONENT_TYPE_SPRITE_2D,      "Sprite2D",    &SceneSchemaLoader::load_sprite_2d,      &SceneSchemaSaver::save_sprite_2d},
    {COMPONENT_TYPE_SCREEN_UI,      "ScreenUI",    nullptr,                                           nullptr},
};
// clang-format on

static_assert(sizeof(sSceneSchemaTable) / sizeof(*sSceneSchemaTable) == COMPONENT_TYPE_ENUM_COUNT);

ComponentView SceneSchemaLoader::load(SceneSchemaLoader& loader, std::string& err)
{
    TOMLReader reader = loader.mReader;
    LD_ASSERT(loader.mScene && reader);

    if (!reader.is_table_scope())
        return {};

    std::string type;
    if (!reader.read_string(SCENE_SCHEMA_KEY_COMPONENT_TYPE, type))
        return {};

    std::string name;
    if (!reader.read_string(SCENE_SCHEMA_KEY_COMPONENT_NAME, name))
        return {};

    SUID compSUID;
    if (!reader.read_suid(SCENE_SCHEMA_KEY_COMPONENT_ID, compSUID))
    {
        err = std::format("component missing ID field");
        return {};
    }

    if (compSUID.type() != SERIAL_TYPE_COMPONENT)
    {
        err = std::format("component invalid SUID {}", compSUID);
        return {};
    }

    ComponentView comp{};

    for (int i = 1; i < (int)COMPONENT_TYPE_ENUM_COUNT; i++)
    {
        if (type == sSceneSchemaTable[i].compTypeName)
        {
            LD_ASSERT(sSceneSchemaTable[i].load);
            comp = sSceneSchemaTable[i].load(loader, compSUID, name.c_str());
            LD_ASSERT(comp); // TODO: deserialization error handling.
        }
    }

    AssetID scriptID = 0;
    reader.read_suid(SCENE_SCHEMA_KEY_COMPONENT_SCRIPT_ID, scriptID);

    comp.set_script_asset_id(scriptID);

    return comp;
}

ComponentView SceneSchemaLoader::load_audio_source(SceneSchemaLoader& loader, SUID compSUID, const char* compName)
{
    Scene scene = loader.mScene;
    TOMLReader reader = loader.mReader;
    LD_ASSERT(scene && reader);

    AudioSourceView source(scene.create_component_serial(COMPONENT_TYPE_AUDIO_SOURCE, compName, loader.mIDReg, (SUID)0, compSUID));
    if (!source)
        return {};

    AssetID clipID = 0;
    reader.read_suid(SCENE_SCHEMA_KEY_AUDIO_SOURCE_CLIP_ID, clipID);

    float pan = 0.5f;
    reader.read_f32(SCENE_SCHEMA_KEY_AUDIO_SOURCE_PAN, pan);

    float volumeLinear = 1.0f;
    reader.read_f32(SCENE_SCHEMA_KEY_AUDIO_SOURCE_VOLUME_LINEAR, volumeLinear);

    if (!source.load(clipID, pan, volumeLinear))
        return {};

    return ComponentView(source.data());
}

ComponentView SceneSchemaLoader::load_transform_2d(SceneSchemaLoader& loader, SUID compSUID, const char* compName)
{
    Scene scene = loader.mScene;
    TOMLReader reader = loader.mReader;
    LD_ASSERT(scene && reader);

    Transform2DView view(scene.create_component_serial(COMPONENT_TYPE_TRANSFORM_2D, compName, loader.mIDReg, (SUID)0, compSUID));
    if (!view)
        return {};

    Transform2D transform;
    if (!TOMLUtil::read_transform_2d(reader, SCENE_SCHEMA_KEY_COMPONENT_TRANSFORM, transform))
        return {};

    view.set_transform_2d(transform);

    return view;
}

ComponentView SceneSchemaLoader::load_camera(SceneSchemaLoader& loader, SUID compSUID, const char* compName)
{
    Scene scene = loader.mScene;
    TOMLReader reader = loader.mReader;
    LD_ASSERT(scene && reader);

    CameraView camera(scene.create_component_serial(COMPONENT_TYPE_CAMERA, compName, loader.mIDReg, (SUID)0, compSUID));
    if (!camera)
        return {};

    TransformEx transform{};
    if (!TOMLUtil::read_transform(reader, SCENE_SCHEMA_KEY_COMPONENT_TRANSFORM, transform))
        return {};

    bool isPerspective = false;
    if (!reader.read_bool(SCENE_SCHEMA_KEY_CAMERA_IS_PERSPECTIVE, isPerspective))
        return {};

    bool isMainCamera = false;
    if (!reader.read_bool(SCENE_SCHEMA_KEY_CAMERA_IS_MAIN, isMainCamera))
        return {};

    if (isPerspective)
    {
        if (!reader.enter_table(SCENE_SCHEMA_TABLE_CAMERA_PERSPECTIVE))
            return {}; // missing perspective info

        float fovDegrees;
        CameraPerspectiveInfo perspective{};
        if (!reader.read_f32(SCENE_SCHEMA_KEY_CAMERA_PERSPECTIVE_FOV, fovDegrees) ||
            !reader.read_f32(SCENE_SCHEMA_KEY_CAMERA_PERSPECTIVE_NEAR_CLIP, perspective.nearClip) ||
            !reader.read_f32(SCENE_SCHEMA_KEY_CAMERA_PERSPECTIVE_FAR_CLIP, perspective.farClip))
        {
            reader.exit();
            return {};
        }

        perspective.fov = LD_TO_RADIANS(fovDegrees);
        perspective.aspectRatio = 1.0f; // overridden later

        if (!camera.load_perspective(perspective))
        {
            reader.exit();
            return {};
        }

        reader.exit();
    }
    else
    {
        if (!reader.enter_table(SCENE_SCHEMA_TABLE_CAMERA_ORTHOGRAPHIC))
            return {}; // missing orthographic info

        CameraOrthographicInfo ortho{};
        if (!reader.read_f32(SCENE_SCHEMA_KEY_CAMERA_ORTHOGRAPHIC_LEFT, ortho.left) ||
            !reader.read_f32(SCENE_SCHEMA_KEY_CAMERA_ORTHOGRAPHIC_RIGHT, ortho.right) ||
            !reader.read_f32(SCENE_SCHEMA_KEY_CAMERA_ORTHOGRAPHIC_BOTTOM, ortho.bottom) ||
            !reader.read_f32(SCENE_SCHEMA_KEY_CAMERA_ORTHOGRAPHIC_TOP, ortho.top) ||
            !reader.read_f32(SCENE_SCHEMA_KEY_CAMERA_ORTHOGRAPHIC_NEAR_CLIP, ortho.nearClip) ||
            !reader.read_f32(SCENE_SCHEMA_KEY_CAMERA_ORTHOGRAPHIC_FAR_CLIP, ortho.farClip) ||
            !camera.load_orthographic(ortho))
        {
            reader.exit();
            return {};
        }

        reader.exit();
    }

    camera.set_transform(transform);

    return ComponentView(camera.data());
}

ComponentView SceneSchemaLoader::load_camera_2d(SceneSchemaLoader& loader, SUID compSUID, const char* compName)
{
    Scene scene = loader.mScene;
    TOMLReader reader = loader.mReader;
    LD_ASSERT(scene && reader);

    Camera2DView camera(scene.create_component_serial(COMPONENT_TYPE_CAMERA_2D, compName, loader.mIDReg, (SUID)0, compSUID));
    if (!camera)
        return {};

    Camera2DInfo info{};
    if (!TOMLUtil::read_vec2(reader, SCENE_SCHEMA_KEY_CAMERA_2D_EXTENT, info.extent))
        return {};

    if (!reader.read_f32(SCENE_SCHEMA_KEY_CAMERA_2D_ZOOM, info.zoom))
        return {};

    Transform2D transform{};
    if (!TOMLUtil::read_transform_2d(reader, SCENE_SCHEMA_KEY_COMPONENT_TRANSFORM, transform))
        return {};

    info.position = transform.position;
    info.rotation = transform.rotation;

    Rect viewport;
    if (!TOMLUtil::read_rect(reader, SCENE_SCHEMA_KEY_CAMERA_2D_VIEWPORT, viewport))
        return {};

    std::string err;
    if (!camera.load(info, viewport, err))
        return {};

    camera.set_transform_2d(transform);

    return ComponentView(camera.data());
}

#if 0
ComponentView SceneSchemaLoader::load_mesh(SceneSchemaLoader& loader, SUID compSUID, const char* compName)
{
    Scene scene = loader.mScene;
    TOMLReader reader = loader.mReader;
    LD_ASSERT(scene && reader);

    MeshView mesh(scene.create_component_serial(COMPONENT_TYPE_MESH, compName, loader.mIDReg, (SUID)0, compSUID));
    if (!mesh)
        return {};

    TransformEx transform;
    if (!TOMLUtil::read_transform(reader, SCENE_SCHEMA_KEY_COMPONENT_TRANSFORM, transform))
        return {};

    if (!mesh.load())
        return {};

    mesh.set_transform(transform);

    AssetID assetID = 0;
    reader.read_suid(SCENE_SCHEMA_KEY_MESH_MESH_ID, assetID);

    mesh.set_mesh_asset(assetID);

    return ComponentView(mesh.data());
}
#endif

ComponentView SceneSchemaLoader::load_sprite_2d(SceneSchemaLoader& loader, SUID compSUID, const char* compName)
{
    Scene scene = loader.mScene;
    TOMLReader reader = loader.mReader;

    Sprite2DView sprite(scene.create_component_serial(COMPONENT_TYPE_SPRITE_2D, compName, loader.mIDReg, (SUID)0, compSUID));
    if (!sprite)
        return {};

    SUID screenLayer = 0;
    reader.read_suid(SCENE_SCHEMA_KEY_SPRITE_2D_SCREEN_LAYER_ID, screenLayer);

    AssetID textureID = 0;
    reader.read_suid(SCENE_SCHEMA_KEY_SPRITE_2D_TEXTURE_2D_ID, textureID);

    if (!sprite.load(screenLayer, textureID))
        return {};

    Rect region;
    if (!TOMLUtil::read_rect(reader, SCENE_SCHEMA_KEY_SPRITE_2D_REGION, region))
        return {};
    sprite.set_region(region);

    Vec2 pivot{};
    if (!TOMLUtil::read_vec2(reader, SCENE_SCHEMA_KEY_SPRITE_2D_PIVOT, pivot))
        return {};
    sprite.set_pivot(pivot);

    Transform2D transform;
    if (!TOMLUtil::read_transform_2d(reader, SCENE_SCHEMA_KEY_COMPONENT_TRANSFORM, transform))
        return {};

    sprite.set_transform_2d(transform);

    uint32_t zDepth = 0;
    reader.read_u32(SCENE_SCHEMA_KEY_SPRITE_2D_Z_DEPTH, zDepth);

    sprite.set_z_depth(zDepth);

    return ComponentView(sprite.data());
}

#if 0
ComponentView SceneSchemaLoader::load_screen_ui(SceneSchemaLoader& loader, SUID compSUID, const char* compName)
{
    Scene scene = loader.mScene;
    TOMLReader reader = loader.mReader;

    ScreenUIView ui(scene.create_component_serial(COMPONENT_TYPE_SCREEN_UI, compName, loader.mIDReg, (SUID)0, compSUID));
    if (!ui)
        return {};

    AssetID uiTemplateID = 0;
    reader.read_suid(SCENE_SCHEMA_KEY_SCREEN_UI_UI_TEMPLATE_ID, uiTemplateID);

    if (!ui.load(uiTemplateID))
        return {};

    return ComponentView(ui.data());
}
#endif

SceneSchemaSaver::~SceneSchemaSaver()
{
    if (mWriter)
        TOMLWriter::destroy(mWriter);
}

bool SceneSchemaSaver::save_scene(Scene scene, std::string& toml, std::string& err)
{
    mScene = scene;

    mWriter = TOMLWriter::create();

    mWriter.begin();

    mWriter.begin_table(SCENE_SCHEMA_TABLE_LUDENS_SCENE);
    mWriter.key(SCENE_SCHEMA_KEY_VERSION_MAJOR).write_i32(LD_VERSION_MAJOR);
    mWriter.key(SCENE_SCHEMA_KEY_VERSION_MINOR).write_i32(LD_VERSION_MINOR);
    mWriter.key(SCENE_SCHEMA_KEY_VERSION_PATCH).write_i32(LD_VERSION_PATCH);
    mWriter.end_table();

    mWriter.begin_array_table(SCENE_SCHEMA_TABLE_COMPONENT);

    Vector<ComponentView> roots;
    mScene.get_root_components(roots);
    for (ComponentView root : roots)
    {
        save(*this, root);
    }

    mWriter.end_array_table();

    mWriter.begin_table(SCENE_SCHEMA_TABLE_HIERARCHY);
    for (auto ite : mChildMap)
    {
        std::string parentID = std::to_string((uint32_t)ite.first);
        mWriter.key(parentID.c_str()).begin_array();

        for (SUID childrenID : ite.second)
            mWriter.write_u32(childrenID);

        mWriter.end_array();
    }
    mWriter.end_table();
    mWriter.end(toml);

    TOMLWriter::destroy(mWriter);
    mWriter = {};

    return true;
}

bool SceneSchemaSaver::save_audio_source(SceneSchemaSaver& saver, ComponentView comp)
{
    LD_ASSERT(saver.mScene && saver.mWriter && comp);

    AudioSourceView source(comp);
    if (!source)
        return false;

    TOMLWriter writer = saver.mWriter;
    writer.key(SCENE_SCHEMA_KEY_AUDIO_SOURCE_CLIP_ID).write_u32(source.get_clip_asset());
    writer.key(SCENE_SCHEMA_KEY_AUDIO_SOURCE_PAN).write_f32(source.get_pan());
    writer.key(SCENE_SCHEMA_KEY_AUDIO_SOURCE_VOLUME_LINEAR).write_f32(source.get_volume_linear());

    return true;
}

bool SceneSchemaSaver::save_transform_2d(SceneSchemaSaver& saver, ComponentView comp)
{
    LD_ASSERT(saver.mScene && saver.mWriter && comp);

    if (!comp)
        return false;

    TOMLWriter writer = saver.mWriter;
    Transform2D transform;
    if (!comp.get_transform_2d(transform) || !TOMLUtil::write_transform_2d(writer, SCENE_SCHEMA_KEY_COMPONENT_TRANSFORM, transform))
        return false;

    return true;
}

bool SceneSchemaSaver::save_camera(SceneSchemaSaver& saver, ComponentView comp)
{
    LD_ASSERT(saver.mScene && saver.mWriter && comp);

    CameraView camera(comp);
    if (!camera)
        return false;

    bool ok;
    TOMLWriter writer = saver.mWriter;
    TransformEx transform;
    camera.get_transform(transform);
    TOMLUtil::write_transform(writer, SCENE_SCHEMA_KEY_COMPONENT_TRANSFORM, transform);

    writer.key(SCENE_SCHEMA_KEY_CAMERA_IS_PERSPECTIVE).write_bool(camera.is_perspective());
    writer.key(SCENE_SCHEMA_KEY_CAMERA_IS_MAIN).write_bool(camera.is_main_camera());

    if (camera.is_perspective())
    {
        writer.begin_inline_table(SCENE_SCHEMA_TABLE_CAMERA_PERSPECTIVE);

        CameraPerspectiveInfo perspective;
        ok = camera.get_perspective_info(perspective);
        LD_ASSERT(ok);

        float fovDegrees = (float)LD_TO_DEGREES(perspective.fov);
        writer.key(SCENE_SCHEMA_KEY_CAMERA_PERSPECTIVE_FOV).write_f32(fovDegrees);
        writer.key(SCENE_SCHEMA_KEY_CAMERA_PERSPECTIVE_FAR_CLIP).write_f32(perspective.farClip);
        writer.key(SCENE_SCHEMA_KEY_CAMERA_PERSPECTIVE_NEAR_CLIP).write_f32(perspective.nearClip);

        writer.end_inline_table();
    }
    else
    {
        writer.begin_inline_table(SCENE_SCHEMA_TABLE_CAMERA_ORTHOGRAPHIC);

        CameraOrthographicInfo ortho;
        ok = camera.get_orthographic_info(ortho);
        LD_ASSERT(ok);

        writer.key(SCENE_SCHEMA_KEY_CAMERA_ORTHOGRAPHIC_LEFT).write_f32(ortho.left);
        writer.key(SCENE_SCHEMA_KEY_CAMERA_ORTHOGRAPHIC_RIGHT).write_f32(ortho.right);
        writer.key(SCENE_SCHEMA_KEY_CAMERA_ORTHOGRAPHIC_BOTTOM).write_f32(ortho.bottom);
        writer.key(SCENE_SCHEMA_KEY_CAMERA_ORTHOGRAPHIC_TOP).write_f32(ortho.top);
        writer.key(SCENE_SCHEMA_KEY_CAMERA_ORTHOGRAPHIC_NEAR_CLIP).write_f32(ortho.nearClip);
        writer.key(SCENE_SCHEMA_KEY_CAMERA_ORTHOGRAPHIC_FAR_CLIP).write_f32(ortho.farClip);

        writer.end_inline_table();
    }

    return true;
}

bool SceneSchemaSaver::save_camera_2d(SceneSchemaSaver& saver, ComponentView comp)
{
    LD_ASSERT(saver.mScene && saver.mWriter && comp);

    Camera2DView camera(comp);
    if (!camera)
        return false;

    Camera2DInfo info = camera.get_info();
    TOMLWriter writer = saver.mWriter;
    Transform2D transform{};
    (void)camera.get_transform_2d(transform);

    if (!TOMLUtil::write_transform_2d(writer, SCENE_SCHEMA_KEY_COMPONENT_TRANSFORM, transform) ||
        !TOMLUtil::write_vec2(writer, SCENE_SCHEMA_KEY_CAMERA_2D_EXTENT, info.extent) ||
        !TOMLUtil::write_rect(writer, SCENE_SCHEMA_KEY_CAMERA_2D_VIEWPORT, camera.get_viewport()) ||
        !writer.key(SCENE_SCHEMA_KEY_CAMERA_2D_ZOOM).write_f32(info.zoom))
        return false;

    return true;
}

#if 0
bool SceneSchemaSaver::save_mesh(SceneSchemaSaver& saver, ComponentView comp)
{
    LD_ASSERT(saver.mScene && saver.mWriter && comp);

    MeshView mesh(comp);
    if (!comp)
        return false;

    TOMLWriter writer = saver.mWriter;
    TransformEx transform;
    mesh.get_transform(transform);
    TOMLUtil::write_transform(writer, SCENE_SCHEMA_KEY_COMPONENT_TRANSFORM, transform);
    writer.key(SCENE_SCHEMA_KEY_MESH_MESH_ID).write_u32(mesh.get_mesh_asset());

    return true;
}
#endif

bool SceneSchemaSaver::save_sprite_2d(SceneSchemaSaver& saver, ComponentView comp)
{
    LD_ASSERT(saver.mScene && saver.mWriter && comp);

    Sprite2DView sprite(comp);
    if (!sprite)
        return false;

    TOMLWriter writer = saver.mWriter;
    TOMLUtil::write_rect(writer, SCENE_SCHEMA_KEY_SPRITE_2D_REGION, sprite.get_region());
    TOMLUtil::write_vec2(writer, SCENE_SCHEMA_KEY_SPRITE_2D_PIVOT, sprite.get_pivot());

    Transform2D transform;
    sprite.get_transform_2d(transform);
    TOMLUtil::write_transform_2d(writer, SCENE_SCHEMA_KEY_COMPONENT_TRANSFORM, transform);

    writer.key(SCENE_SCHEMA_KEY_SPRITE_2D_SCREEN_LAYER_ID).write_u32(sprite.get_screen_layer_suid());
    writer.key(SCENE_SCHEMA_KEY_SPRITE_2D_TEXTURE_2D_ID).write_u32(sprite.get_texture_2d_asset());
    writer.key(SCENE_SCHEMA_KEY_SPRITE_2D_Z_DEPTH).write_u32(sprite.get_z_depth());

    return true;
}

#if 0
bool SceneSchemaSaver::save_screen_ui(SceneSchemaSaver& saver, ComponentView comp)
{
    LD_ASSERT(saver.mScene && saver.mWriter && comp);

    ScreenUIView ui(comp);
    if (!ui)
        return false;

    TOMLWriter writer = saver.mWriter;
    writer.key(SCENE_SCHEMA_KEY_SCREEN_UI_UI_TEMPLATE_ID).write_u32(ui.get_ui_template_asset());

    return true;
}
#endif

void SceneSchemaSaver::save(SceneSchemaSaver& saver, ComponentView root)
{
    LD_ASSERT(root);

    TOMLWriter writer = saver.mWriter;

    writer.begin_table();

    ComponentType type = root.type();
    std::string compTypeName(sSceneSchemaTable[(int)type].compTypeName);
    writer.key(SCENE_SCHEMA_KEY_COMPONENT_ID).write_u32(root.suid());
    writer.key(SCENE_SCHEMA_KEY_COMPONENT_TYPE).write_string(compTypeName);
    writer.key(SCENE_SCHEMA_KEY_COMPONENT_NAME).write_string(root.get_name());
    writer.key(SCENE_SCHEMA_KEY_COMPONENT_SCRIPT_ID).write_u32(root.get_script_asset_id());

    // TODO: Transform or Transform2D could be saved here?

    LD_ASSERT(sSceneSchemaTable[(int)type].save);
    bool ok = sSceneSchemaTable[(int)type].save(saver, root);
    LD_ASSERT(ok); // TODO: error handling path

    writer.end_table();

    // recursively save entire subtree
    Vector<ComponentView> children;
    root.get_children(children);

    for (ComponentView child : children)
    {
        LD_ASSERT(child);
        saver.mChildMap[root.suid()].push_back(child.suid());
        save(saver, child);
    }
}

SceneSchemaLoader::~SceneSchemaLoader()
{
    if (mReader)
        TOMLReader::destroy(mReader);
}

bool SceneSchemaLoader::load_scene(Scene scene, SUIDRegistry idReg, const View& toml, std::string& err)
{
    mScene = scene;
    mReader = TOMLReader::create(toml, err);
    mIDReg = idReg;

    if (!mReader)
        return false;

    scene.reset();

    if (!mReader.enter_table(SCENE_SCHEMA_TABLE_LUDENS_SCENE))
        return false;

    uint32_t version;
    if (!mReader.read_u32(SCENE_SCHEMA_KEY_VERSION_MAJOR, version) || version != LD_VERSION_MAJOR)
        return false;

    if (!mReader.read_u32(SCENE_SCHEMA_KEY_VERSION_MINOR, version) || version != LD_VERSION_MINOR)
        return false;

    if (!mReader.read_u32(SCENE_SCHEMA_KEY_VERSION_PATCH, version) || version != LD_VERSION_PATCH)
        return false;

    mReader.exit();

    bool valid = true;

    // extract component tables
    int componentCount = 0;
    if (mReader.enter_array(SCENE_SCHEMA_TABLE_COMPONENT, componentCount))
    {
        for (int i = 0; valid && i < componentCount; i++)
        {
            if (!mReader.enter_table(i))
                continue;

            ComponentView comp = load(*this, err);
            if (!comp)
                valid = false;

            mReader.exit();
        }

        mReader.exit();
    }

    if (valid && mReader.enter_table(SCENE_SCHEMA_TABLE_HIERARCHY))
    {
        Vector<std::string> keys;
        mReader.get_keys(keys);

        for (const std::string& key : keys)
        {
            SUID parentSUID((uint32_t)std::stoul(key, nullptr, 0));

            if (parentSUID.type() != SERIAL_TYPE_COMPONENT)
            {
                err = std::format("found invalid component SUID {}", parentSUID);
                valid = false;
                continue;
            }

            int childrenCount = 0;
            if (!mReader.enter_array(key.c_str(), childrenCount))
                continue;

            for (int i = 0; i < childrenCount; i++)
            {
                SUID childSUID;
                if (!mReader.read_suid(i, childSUID))
                    continue;

                if (childSUID.type() != SERIAL_TYPE_COMPONENT)
                {
                    err = std::format("found invalid component SUID {}", childSUID);
                    valid = false;
                    continue;
                }

                ComponentView child = mScene.get_component_by_suid(childSUID);
                ComponentView parent = mScene.get_component_by_suid(parentSUID);

                if (child && parent)
                    scene.reparent_component_subtree(child.cuid(), parent.cuid());
            }

            mReader.exit();
        }

        mReader.exit();
    }

    TOMLReader::destroy(mReader);
    mReader = {};

    return valid;
}

//
// Public API
//

bool SceneSchema::load_scene_from_source(Scene scene, SUIDRegistry idRegistry, const View& toml, std::string& err)
{
    LD_PROFILE_SCOPE;

    SceneSchemaLoader loader;
    if (!loader.load_scene(scene, idRegistry, toml, err))
        return false;

    return true;
}

bool SceneSchema::load_scene_from_file(Scene scene, SUIDRegistry idRegistry, const FS::Path& tomlPath, std::string& err)
{
    LD_PROFILE_SCOPE;

    Vector<byte> toml;
    if (!FS::read_file_to_vector(tomlPath, toml, err))
        return false;

    return load_scene_from_source(scene, idRegistry, view(toml), err);
}

bool SceneSchema::save_scene(Scene scene, const FS::Path& savePath, std::string& err)
{
    LD_PROFILE_SCOPE;

    std::string toml;
    SceneSchemaSaver saver;
    if (!saver.save_scene(scene, toml, err))
        return false;

    return FS::write_file_and_swap_backup(savePath, view(toml), err);
}

std::string SceneSchema::create_empty()
{
    TOMLWriter writer = TOMLWriter::create();

    writer.begin();

    writer.begin_table(SCENE_SCHEMA_TABLE_LUDENS_SCENE);
    writer.key(SCENE_SCHEMA_KEY_VERSION_MAJOR).write_i32(LD_VERSION_MAJOR);
    writer.key(SCENE_SCHEMA_KEY_VERSION_MINOR).write_i32(LD_VERSION_MINOR);
    writer.key(SCENE_SCHEMA_KEY_VERSION_PATCH).write_i32(LD_VERSION_PATCH);
    writer.end_table();

    std::string toml;
    writer.end(toml);

    TOMLWriter::destroy(writer);

    return toml;
}

} // namespace LD
