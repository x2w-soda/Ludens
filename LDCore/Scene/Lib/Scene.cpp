#include <Ludens/Asset/AssetType/UITemplateAsset.h>
#include <Ludens/Camera/Camera.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Math.h>
#include <Ludens/Lua/LuaModule.h>
#include <Ludens/Lua/LuaState.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Scene.h>

// Scene implementation may know about DataRegistry memory layouts.
#include <Ludens/DataRegistry/DataComponent.h>

#include <iostream>

#include "AudioSystemCache.h"
#include "LuaScript.h"
#include "RenderSystemCache.h"
#include "SceneObj.h"

// Scene user's responsibility to check handle before calling methods
#define LD_ASSERT_COMPONENT_LOADED(DATA) LD_ASSERT(DATA && *(DATA) && ((*(DATA))->flags & COMPONENT_FLAG_LOADED_BIT))

namespace LD {

/// @brief Scene Singleton, all scene operations including transition should be done in-place,
///        the SceneObj address should be immutable.
SceneObj* sScene = nullptr;

static bool load_audio_source_component(SceneObj* scene, AudioSourceComponent* source, AssetID clipID, float pan, float volumeLinear);
static bool clone_audio_source_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData);
static void unload_audio_source_component(SceneObj* scene, ComponentBase** source);
static void cleanup_audio_source_component(SceneObj* scene, ComponentBase** source);
static bool load_camera_component_perspective(SceneObj* scene, CameraComponent* camera, const CameraPerspectiveInfo& perspectiveI);
static bool load_camera_component_orthographic(SceneObj* scene, CameraComponent* camera, const CameraOrthographicInfo& perspectiveI);
static bool clone_camera_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData);
static void unload_camera_component(SceneObj* scene, ComponentBase** camera);
static void startup_camera_component(SceneObj* scene, ComponentBase** data);
static void cleanup_camera_component(SceneObj* scene, ComponentBase** data);
static bool load_mesh_component(SceneObj* scene, MeshComponent* mesh, AssetID meshAID);
static bool clone_mesh_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData);
static void unload_mesh_component(SceneObj* scene, ComponentBase** data);
static bool load_sprite_2d_component_suid(SceneObj* scene, Sprite2DComponent* sprite, SUID layerSUID, AssetID texture2D);
static bool load_sprite_2d_component_ruid(SceneObj* scene, Sprite2DComponent* sprite, RUID layerRUID, AssetID texture2D);
static bool clone_sprite_2d_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData);
static void unload_sprite_2d_component(SceneObj* scene, ComponentBase** data);
static bool load_screen_ui_component(SceneObj* scene, ScreenUIComponent* ui, AssetID uiTemplateID);
static bool clone_screen_ui_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData);
static void unload_screen_ui_component(SceneObj* scene, ComponentBase** data);
static void startup_screen_ui_component(SceneObj* scene, ComponentBase** data);
static void cleanup_screen_ui_component(SceneObj* scene, ComponentBase** data);

/// @brief Component behavior and operations within a Scene.
///        - loading a component creates subsystem resources
///        - cloning loads an empty component from some loaded component
///        - startup a component to prepare it for runtime
///        - cleanup a component to release runtime resources
///        - unloading a component destroys subsystem resources
struct SceneComponent
{
    ComponentType type;
    bool (*clone)(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData);
    void (*unload)(SceneObj* scene, ComponentBase** data);
    void (*startup)(SceneObj* scene, ComponentBase** data);
    void (*cleanup)(SceneObj* scene, ComponentBase** data);
};

// clang-format off
static SceneComponent sSceneComponents[] = {
    {COMPONENT_TYPE_DATA,         nullptr,                       nullptr,                        nullptr,                      nullptr},
    {COMPONENT_TYPE_AUDIO_SOURCE, &clone_audio_source_component, &unload_audio_source_component, nullptr,                      &cleanup_audio_source_component},
    {COMPONENT_TYPE_TRANSFORM,    nullptr,                       nullptr,                        nullptr,                      nullptr},
    {COMPONENT_TYPE_CAMERA,       &clone_camera_component,       &unload_camera_component,       &startup_camera_component,    &cleanup_camera_component},
    {COMPONENT_TYPE_MESH,         &clone_mesh_component,         &unload_mesh_component,         nullptr,                      nullptr},
    {COMPONENT_TYPE_SPRITE_2D,    &clone_sprite_2d_component,    &unload_sprite_2d_component,    nullptr,                      nullptr},
    {COMPONENT_TYPE_SCREEN_UI,    &clone_screen_ui_component,    &unload_screen_ui_component,    &startup_screen_ui_component, &cleanup_screen_ui_component},
};
// clang-format on

static_assert(sizeof(sSceneComponents) / sizeof(*sSceneComponents) == COMPONENT_TYPE_ENUM_COUNT);

//
// IMPLEMENTATION
//

static bool load_audio_source_component(SceneObj* scene, AudioSourceComponent* source, AssetID clipID, float pan, float volumeLinear)
{
    LD_PROFILE_SCOPE;

    // NOTE: Buffer not destroyed upon component unload.
    //       Other components may still be using it for playback.
    AudioBuffer buffer = scene->audioSystemCache.get_or_create_audio_buffer(clipID);
    if (!buffer)
        return false;

    source->pan = pan;
    source->volumeLinear = volumeLinear;
    source->playback = scene->audioSystemCache.create_playback(buffer, pan, volumeLinear);
    if (!source->playback)
        return false;

    source->clipID = clipID;
    source->base->flags |= COMPONENT_FLAG_LOADED_BIT;
    return true;
}

static bool clone_audio_source_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData)
{
    LD_PROFILE_SCOPE;

    Scene::AudioSource srcSource(srcData);
    Scene::AudioSource dstSource(dstData);
    LD_ASSERT(srcSource && dstSource);

    AssetID clipAID = srcSource.get_clip_asset();
    float pan = srcSource.get_pan();
    float volume = srcSource.get_volume_linear();

    return load_audio_source_component(scene, (AudioSourceComponent*)dstSource.data(), clipAID, pan, volume);
}

static void unload_audio_source_component(SceneObj* scene, ComponentBase** sourceData)
{
    auto* source = (AudioSourceComponent*)sourceData;

    if (source->playback)
    {
        scene->audioSystemCache.destroy_playback(source->playback);
        source->playback = {};
    }

    // NOTE: audio buffer still exists in audio system cache
    source->base->flags &= ~COMPONENT_FLAG_LOADED_BIT;
}

static void cleanup_audio_source_component(SceneObj* scene, ComponentBase** sourceData)
{
    auto* source = (AudioSourceComponent*)sourceData;

    if (source->playback)
    {
        scene->audioSystemCache.stop_playback(source->playback);
        source->playback = {};
    }
}

static bool load_camera_component_perspective(SceneObj* scene, CameraComponent* camera, const CameraPerspectiveInfo& perspectiveI)
{
    LD_PROFILE_SCOPE;

    camera->camera = LD::Camera::create(perspectiveI, Vec3(0.0f));

    if (!camera->camera)
        return false;

    camera->base->flags |= COMPONENT_FLAG_LOADED_BIT;
    return true;
}

static bool load_camera_component_orthographic(SceneObj* scene, CameraComponent* camera, const CameraOrthographicInfo& perspectiveI)
{
    LD_PROFILE_SCOPE;

    camera->camera = LD::Camera::create(perspectiveI, Vec3(0.0f));

    if (!camera->camera)
        return false;

    camera->base->flags |= COMPONENT_FLAG_LOADED_BIT;
    return true;
}

static bool clone_camera_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData)
{
    LD_PROFILE_SCOPE;

    Scene::Camera srcCamera(srcData);
    LD_ASSERT(srcCamera);

    CameraPerspectiveInfo perspectiveI;
    CameraOrthographicInfo orthoI;
    if (srcCamera.is_perspective())
    {
        if (!srcCamera.get_perspective_info(perspectiveI) || !load_camera_component_perspective(scene, (CameraComponent*)dstData, perspectiveI))
            return false;
    }
    else
    {
        if (!srcCamera.get_orthographic_info(orthoI) || !load_camera_component_orthographic(scene, (CameraComponent*)dstData, orthoI))
            return false;
    }

    if (srcCamera.is_main_camera())
        ((CameraComponent*)dstData)->isMainCamera = true;

    return true;
}

static void unload_camera_component(SceneObj* scene, ComponentBase** cameraData)
{
    CameraComponent* camera = (CameraComponent*)cameraData;

    if (camera->camera)
    {
        LD::Camera::destroy(camera->camera);
        camera->camera = {};
    }

    camera->base->flags &= ~COMPONENT_FLAG_LOADED_BIT;
}

static void startup_camera_component(SceneObj* scene, ComponentBase** cameraData)
{
    ComponentBase* base = *cameraData;
    auto* camera = (CameraComponent*)cameraData;

    if (scene->mainCameraC)
        return;

    scene->mainCameraC = camera;

    const Vec3 mainCameraTarget(0.0f, 0.0f, 1.0f);
}

static void cleanup_camera_component(SceneObj* scene, ComponentBase** cameraData)
{
    if (scene->mainCameraC == (CameraComponent*)cameraData)
        scene->mainCameraC = nullptr;
}

static bool load_mesh_component(SceneObj* scene, MeshComponent* mesh, AssetID meshAID)
{
    LD_PROFILE_SCOPE;

    ComponentBase* base = mesh->base;

    mesh->draw = scene->renderSystemCache.create_mesh_draw(base->cuid, meshAID);

    if (!mesh->draw)
        return false;

    mesh->assetID = meshAID;

    base->flags |= COMPONENT_FLAG_LOADED_BIT;
    return true;
}

static bool clone_mesh_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData)
{
    LD_PROFILE_SCOPE;

    Scene::Mesh dstMesh(dstData);
    Scene::Mesh srcMesh(srcData);
    LD_ASSERT(dstMesh && srcMesh);

    AssetID srcMeshAID = srcMesh.get_mesh_asset();

    return load_mesh_component(scene, (MeshComponent*)dstData, srcMeshAID);
}

static void unload_mesh_component(SceneObj* scene, ComponentBase** data)
{
    MeshComponent* mesh = (MeshComponent*)data;
    ComponentBase* base = mesh->base;

    LD_ASSERT(mesh->draw);
    scene->renderSystemCache.destroy_mesh_draw(mesh->draw);
    mesh->draw = {};

    base->flags &= ~COMPONENT_FLAG_LOADED_BIT;
}

static bool load_sprite_2d_component_suid(SceneObj* scene, Sprite2DComponent* sprite, SUID layerSUID, AssetID texture2D)
{
    LD_PROFILE_SCOPE;

    ComponentBase* base = sprite->base;

    RUID layerRUID = scene->renderSystemCache.get_or_create_screen_layer(layerSUID);
    if (!layerRUID)
        return false;

    return load_sprite_2d_component_ruid(scene, sprite, layerRUID, texture2D);
}

static bool load_sprite_2d_component_ruid(SceneObj* scene, Sprite2DComponent* sprite, RUID layerRUID, AssetID texture2D)
{
    LD_PROFILE_SCOPE;

    ComponentBase* base = sprite->base;

    sprite->draw = scene->renderSystemCache.create_sprite_2d_draw(base->cuid, layerRUID, texture2D);
    if (!sprite->draw)
        return false;

    sprite->assetID = texture2D;

    base->flags |= COMPONENT_FLAG_LOADED_BIT;
    return true;
}

static bool clone_sprite_2d_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData)
{
    LD_PROFILE_SCOPE;

    Scene::Sprite2D srcSprite((Sprite2DComponent*)srcData);
    Scene::Sprite2D dstSprite((Sprite2DComponent*)dstData);
    LD_ASSERT(srcSprite && dstSprite);

    RUID layerRUID = srcSprite.get_screen_layer_ruid();
    AssetID texture2D = srcSprite.get_texture_2d_asset();

    if (!load_sprite_2d_component_ruid(scene, (Sprite2DComponent*)dstData, layerRUID, texture2D))
        return false;

    dstSprite.set_pivot(srcSprite.get_pivot());
    dstSprite.set_region(srcSprite.get_region());
    dstSprite.set_z_depth(srcSprite.get_z_depth());

    return true;
}

static void unload_sprite_2d_component(SceneObj* scene, ComponentBase** data)
{
    Sprite2DComponent* sprite = (Sprite2DComponent*)data;
    ComponentBase* base = sprite->base;

    if (sprite->draw)
    {
        scene->renderSystemCache.destroy_sprite_2d_draw(sprite->draw);
        sprite->draw = {};
    }

    base->flags &= ~COMPONENT_FLAG_LOADED_BIT;
}

static bool load_screen_ui_component(SceneObj* scene, ScreenUIComponent* ui, AssetID uiTemplateID)
{
    LD_PROFILE_SCOPE;

    ComponentBase* base = ui->base;

    UITemplateAsset asset = (UITemplateAsset)scene->assetManager.get_asset(uiTemplateID);
    if (!asset || asset.get_type() != ASSET_TYPE_UI_TEMPLATE)
        return false;

    UILayoutInfo layoutI{};
    UIWindowInfo windowI{};
    UIWorkspace space = scene->screenUI.workspace();
    ui->uiWindow = space.create_window(layoutI, windowI, nullptr);

    if (!ui->uiWindow || !asset.load_ui_subtree(ui->uiWindow, nullptr, nullptr))
        return false;

    ui->uiTemplateID = uiTemplateID;

    base->flags |= COMPONENT_FLAG_LOADED_BIT;
    return true;
}

static bool clone_screen_ui_component(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData)
{
    LD_PROFILE_SCOPE;

    Scene::ScreenUI srcUI((ScreenUIComponent*)srcData);
    Scene::ScreenUI dstUI((ScreenUIComponent*)dstData);
    LD_ASSERT(srcUI && dstUI);

    AssetID uiTemplateID = srcUI.get_ui_template_asset();

    return load_screen_ui_component(scene, (ScreenUIComponent*)dstData, uiTemplateID);
}

static void unload_screen_ui_component(SceneObj* scene, ComponentBase** data)
{
    ScreenUIComponent* ui = (ScreenUIComponent*)data;

    UIWorkspace space = scene->screenUI.workspace();
    space.destroy_window(ui->uiWindow);
    ui->uiWindow = {};

    ComponentBase* base = *data;
    base->flags &= ~COMPONENT_FLAG_LOADED_BIT;
}

static void startup_screen_ui_component(SceneObj* scene, ComponentBase** data)
{
    auto* ui = (ScreenUIComponent*)data;

    UITemplateAsset asset = (UITemplateAsset)scene->assetManager.get_asset(ui->uiTemplateID);
    LD_ASSERT(asset && asset.get_type() == ASSET_TYPE_UI_TEMPLATE);

    // TODO: UIDriver attach
}

static void cleanup_screen_ui_component(SceneObj* scene, ComponentBase** data)
{
    auto* ui = (ScreenUIComponent*)data;
    LD_ASSERT(ui);

    // TODO: UIDriver detach
}

void SceneObj::load_registry_from_backup()
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(registry && registryBackup);

    Vector<ComponentBase**> dstRoots;
    Vector<ComponentBase**> srcRoots;
    registry.get_root_component_data(dstRoots);
    registryBackup.get_root_component_data(srcRoots);
    LD_ASSERT(dstRoots.size() == srcRoots.size());

    for (size_t i = 0; i < dstRoots.size(); i++)
    {
        bool ok = load_subtree_from_backup(dstRoots[i], srcRoots[i]);
        LD_ASSERT(ok);
    }
}

void SceneObj::unload_registry()
{
    LD_PROFILE_SCOPE;

    Vector<ComponentBase**> roots;
    registry.get_root_component_data(roots);

    for (ComponentBase** rootData : roots)
    {
        unload_subtree(rootData);
    }
}

void SceneObj::startup_registry()
{
    LD_PROFILE_SCOPE;

    Vector<ComponentBase**> roots;
    registry.get_root_component_data(roots);

    for (ComponentBase** rootData : roots)
    {
        startup_subtree(rootData);
    }
}

void SceneObj::cleanup_registry()
{
    LD_PROFILE_SCOPE;

    Vector<ComponentBase**> roots;
    registry.get_root_component_data(roots);

    for (ComponentBase** rootData : roots)
    {
        cleanup_subtree(rootData);
    }

    mainCameraC = nullptr;
}

// This is basically the editor loading a copy of component subtree from backup data
bool SceneObj::load_subtree_from_backup(ComponentBase** dstData, ComponentBase** srcData)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(dstData && srcData);

    ComponentBase* dstBase = *dstData;
    ComponentBase* srcBase = *srcData;

    // sanity checks
    LD_ASSERT(srcBase->type == dstBase->type);
    LD_ASSERT(srcBase->suid == dstBase->suid);
    LD_ASSERT((srcBase->flags & COMPONENT_FLAG_LOADED_BIT) && ~(dstBase->flags & COMPONENT_FLAG_LOADED_BIT));
    LD_ASSERT((std::string(dstBase->name) == std::string(srcBase->name)));

    bool ok = sSceneComponents[(int)dstBase->type].clone(sScene, dstData, srcData);
    LD_ASSERT(ok);
    if (!ok)
        return false;

    ComponentBase* dstChild = dstBase->child;
    ComponentBase* srcChild = srcBase->child;

    while (dstChild)
    {
        LD_ASSERT(dstChild && srcChild);
        ComponentBase** dstChildData = registry.get_component_data(dstChild->cuid, nullptr);
        ComponentBase** srcChildData = registryBackup.get_component_data(srcChild->cuid, nullptr);

        if (!load_subtree_from_backup(dstChildData, srcChildData))
            return false;

        dstChild = dstChild->next;
        srcChild = srcChild->next;
    }

    return true;
}

void SceneObj::unload_subtree(ComponentBase** data)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(data);
    ComponentBase* base = (*data);
    LD_ASSERT(base->flags & COMPONENT_FLAG_LOADED_BIT);

    if (sSceneComponents[(int)base->type].unload)
        sSceneComponents[(int)base->type].unload(this, data);

    // sanity check
    LD_ASSERT((base->flags & COMPONENT_FLAG_LOADED_BIT) == 0);

    for (ComponentBase* child = base->child; child; child = child->next)
    {
        ComponentBase** childData = registry.get_component_data(child->cuid, nullptr);
        unload_subtree(childData);
    }
}

void SceneObj::startup_subtree(ComponentBase** rootData)
{
    LD_ASSERT(rootData);
    ComponentBase* rootBase = *rootData;
    LD_ASSERT(rootBase->flags & COMPONENT_FLAG_LOADED_BIT);

    for (ComponentBase* childBase = rootBase->child; childBase; childBase = childBase->next)
    {
        ComponentBase** childData = registry.get_component_data(childBase->cuid, nullptr);
        startup_subtree(childData);
    }

    // post-order traversal, all child components of root already have their scripts attached
    if (sSceneComponents[(int)rootBase->type].startup)
        sSceneComponents[(int)rootBase->type].startup(this, rootData);

    if (rootBase->scriptAssetID)
    {
        const CUID rootCUID = rootBase->cuid;
        luaContext.create_component_table(rootCUID);
        bool success = luaContext.create_lua_script(rootCUID, rootBase->scriptAssetID); // TODO: abort startup at the first failure of creating lua script instance.
        LD_ASSERT(success);
        luaContext.attach_lua_script(rootCUID);
    }
}

void SceneObj::cleanup_subtree(ComponentBase** rootData)
{
    LD_ASSERT(rootData);
    ComponentBase* rootBase = *rootData;

    for (ComponentBase* childBase = rootBase->child; childBase; childBase = childBase->next)
    {
        ComponentBase** childData = registry.get_component_data(childBase->cuid, nullptr);
        cleanup_subtree(childData);
    }

    // post-order traversal, all child components of root already have their scripts detached
    if (sSceneComponents[(int)rootBase->type].cleanup)
        sSceneComponents[(int)rootBase->type].cleanup(this, rootData);

    const CUID rootCUID = rootBase->cuid;
    luaContext.detach_lua_script(rootCUID);
    luaContext.destroy_lua_script(rootCUID);
    luaContext.destroy_component_table(rootCUID);
}

//
// PUBLIC API
//

const char* get_lua_script_log_channel_name()
{
    return LuaScript::get_log_channel_name();
}

Scene Scene::create(const SceneInfo& sceneI)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(sScene == nullptr);
    sScene = heap_new<SceneObj>(MEMORY_USAGE_SCENE);
    sScene->registry = DataRegistry::create();
    sScene->assetManager = sceneI.assetManager;
    sScene->renderSystemCache.create(sceneI.renderSystem, sceneI.assetManager);
    sScene->audioSystemCache.create(sceneI.audioSystem, sceneI.assetManager);
    sScene->luaContext.create(Scene(sScene), sceneI.assetManager);
    sScene->screenExtent = {};

    ScreenUIInfo info{};
    info.extent = sScene->screenExtent;
    info.fontAtlas = sceneI.fontAtlas;
    info.fontAtlasImage = sceneI.fontAtlasImage;
    info.theme = sceneI.uiTheme;
    LD_ASSERT(info.fontAtlas);
    LD_ASSERT(info.fontAtlasImage);
    LD_ASSERT(info.theme);
    sScene->screenUI = LD::ScreenUI::create(info);
    sScene->screenUIBackup = {}; // TODO:

    return Scene(sScene);
}

void Scene::destroy(Scene scene)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(sScene && sScene == scene.unwrap());

    // destroy all components
    scene.reset();

    LD::ScreenUI::destroy(sScene->screenUI);
    sScene->luaContext.destroy();
    sScene->audioSystemCache.destroy();
    sScene->renderSystemCache.destroy();
    DataRegistry::destroy(sScene->registry);

    heap_delete<SceneObj>(sScene);
    sScene = nullptr;
}

void Scene::reset()
{
    LD_PROFILE_SCOPE;

    if (mObj->state == SCENE_STATE_LOADED)
        unload();

    LD_ASSERT(mObj->state == SCENE_STATE_EMPTY);
    LD_ASSERT(!mObj->registryBackup);

    if (mObj->registry)
    {
        DataRegistry::destroy(mObj->registry);
        mObj->registry = {};
    }

    mObj->mainCameraC = nullptr;
    mObj->registry = DataRegistry::create();
}

void Scene::load(const std::function<bool(Scene)>& loader)
{
    LD_PROFILE_SCOPE;

    if (mObj->state == SCENE_STATE_LOADED)
    {
        LD_UNREACHABLE;
        return;
    }

    bool success = loader(*this);
    if (!success)
    {
        // TODO: unwind
        LD_UNREACHABLE;
        return;
    }

    /*
    if (!validate())
    {
        // TODO: unwind
        return;
    }
    */

    mObj->state = SCENE_STATE_LOADED;
}

void Scene::unload()
{
    LD_PROFILE_SCOPE;

    if (mObj->state == SCENE_STATE_EMPTY)
        return;

    mObj->unload_registry();

    mObj->state = SCENE_STATE_EMPTY;
}

void Scene::backup()
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(mObj->state == SCENE_STATE_LOADED);
    LD_ASSERT(!mObj->registryBackup);
    LD_ASSERT(!mObj->screenUIBackup);

    mObj->screenUIBackup = mObj->screenUI;
    mObj->screenUI;

    // create and load a copy for play-in-editor session
    DataRegistry pie = mObj->registry.duplicate();
    mObj->registryBackup = mObj->registry;
    mObj->registry = pie;
    mObj->load_registry_from_backup();
}

void Scene::startup()
{
    LD_PROFILE_SCOPE;

    if (mObj->state == SCENE_STATE_RUNNING)
        return;

    mObj->state = SCENE_STATE_RUNNING;

    mObj->startup_registry();
}

void Scene::cleanup()
{
    LD_PROFILE_SCOPE;

    if (mObj->state != SCENE_STATE_RUNNING)
        return;

    mObj->state = SCENE_STATE_LOADED;

    mObj->cleanup_registry();

    // after play-in-editor session, restore the backup.
    if (mObj->registryBackup)
    {
        mObj->unload_registry();
        DataRegistry::destroy(mObj->registry);
        mObj->registry = mObj->registryBackup;
        mObj->registryBackup = {};
    }
}

void Scene::update(const Vec2& screenExtent, float delta)
{
    LD_PROFILE_SCOPE;
    LD_ASSERT(screenExtent.x > 0.0f && screenExtent.y > 0.0f);

    mObj->screenExtent = screenExtent;

    // update all lua script instances
    mObj->luaContext.update(delta);

    // update screen space UI
    mObj->screenUI.update(delta);

    if (mObj->mainCameraC)
    {
        const CameraComponent* cameraC = mObj->mainCameraC;
        const ComponentBase* base = cameraC->base;
        LD::Camera mainCamera = cameraC->camera;
        LD_ASSERT(base && mainCamera);

        Mat4 worldMat4;
        mObj->registry.get_component_world_mat4(base->cuid, worldMat4);
        Vec3 forward = worldMat4.as_mat3() * Vec3(0.0f, 0.0f, 1.0f);

        mainCamera.set_aspect_ratio(screenExtent.x / screenExtent.y);
        mainCamera.set_pos(cameraC->transform.position);
        mainCamera.set_target(cameraC->transform.position + forward);
    }

    // any heap allocations for audio is done on main thread.
    mObj->audioSystemCache.update();
}

Camera Scene::get_camera()
{
    if (mObj->mainCameraC)
        return mObj->mainCameraC->camera;

    return {};
}

Scene::Component Scene::create_component(ComponentType type, const char* name, CUID parentCUID)
{
    CUID compCUID = mObj->registry.create_component(type, name, parentCUID, (SUID)0);

    // TODO: DataRegistry API without CUID -> Component Data chasing.
    return Scene::Component(mObj->registry.get_component_data(compCUID, nullptr));
}

Scene::Component Scene::create_component_serial(ComponentType type, const char* name, SUID parentSUID, SUID hintSUID)
{
    ComponentBase** parentData = mObj->registry.get_component_data_by_suid(parentSUID, nullptr);

    if (parentSUID && !parentData) // bad input, parent does not exist in serial domain
        return {};

    CUID parentCUID = parentData ? (*parentData)->cuid : 0;
    CUID compCUID = mObj->registry.create_component(type, name, parentCUID, hintSUID);

    // TODO: DataRegistry API without CUID -> Component Data chasing.
    return Scene::Component(mObj->registry.get_component_data(compCUID, nullptr));
}

void Scene::destroy_component(CUID compID)
{
    mObj->registry.destroy_component(compID);
}

void Scene::reparent(CUID compID, CUID parentID)
{
    mObj->registry.reparent(compID, parentID);
}

void Scene::get_root_components(Vector<Component>& roots)
{
    roots.clear();

    Vector<ComponentBase**> rootData;
    mObj->registry.get_root_component_data(rootData);

    // kinda slow for pointer chasing CUID > Component Data
    for (ComponentBase** root : rootData)
    {
        Scene::Component comp(root);
        LD_ASSERT(comp);
        roots.push_back(comp);
    }
}

Scene::Component Scene::get_component(CUID compID)
{
    return Scene::Component(mObj->registry.get_component_data(compID, nullptr));
}

Scene::Component Scene::get_component_by_suid(SUID compSUID)
{
    return Scene::Component(mObj->registry.get_component_data_by_suid(compSUID, nullptr));
}

ComponentType Scene::Component::type()
{
    return (*mData)->type;
}

CUID Scene::Component::cuid()
{
    return (*mData)->cuid;
}

SUID Scene::Component::suid()
{
    return (*mData)->suid;
}

RUID Scene::Component::ruid()
{
    return sScene->renderSystemCache.get_component_draw_id((*mData)->cuid);
}

const char* Scene::Component::get_name()
{
    return (*mData)->name;
}

AssetID Scene::Component::get_script_asset_id()
{
    return (*mData)->scriptAssetID;
}

void Scene::Component::set_script_asset_id(AssetID assetID)
{
    (*mData)->scriptAssetID = assetID;
}

void Scene::Component::get_children(Vector<Component>& children)
{
    children.clear();

    // kinda slow with ID -> Data pointer chasing.
    for (ComponentBase* child = (*mData)->child; child; child = child->next)
    {
        Scene::Component childC(sScene->registry.get_component_data(child->cuid, nullptr));
        LD_ASSERT(childC);
        children.push_back(childC);
    }
}

Scene::Component Scene::Component::get_parent()
{
    ComponentBase* parentBase = (*mData)->parent;
    if (!parentBase)
        return {};

    return Scene::Component(sScene->registry.get_component_data(parentBase->cuid, nullptr));
}

bool Scene::Component::get_transform(TransformEx& transform)
{
    // TODO: DataRegistry API to take ComponentBase* directly
    return sScene->registry.get_component_transform((*mData)->cuid, transform);
}

bool Scene::Component::set_transform(const TransformEx& transform)
{
    // TODO: DataRegistry API to take ComponentBase* directly
    return sScene->registry.set_component_transform((*mData)->cuid, transform);
}

bool Scene::Component::get_transform_2d(Transform2D& transform)
{
    // TODO: DataRegistry API to take ComponentBase* directly
    return sScene->registry.get_component_transform_2d((*mData)->cuid, transform);
}

bool Scene::Component::set_transform_2d(const Transform2D& transform)
{
    // TODO: DataRegistry API to take ComponentBase* directly
    return sScene->registry.set_component_transform_2d((*mData)->cuid, transform);
}

bool Scene::Component::get_world_mat4(Mat4& worldMat4)
{
    ComponentBase* base = *mData;

    return sScene->registry.get_component_world_mat4(base->cuid, worldMat4);
}

Scene::Component Scene::get_ruid_component(RUID ruid)
{
    CUID compCUID = mObj->renderSystemCache.get_draw_id_component(ruid);

    return Scene::Component(sScene->registry.get_component_data(compCUID, nullptr));
}

bool Scene::get_ruid_world_mat4(RUID ruid, Mat4& worldMat4)
{
    LD_PROFILE_SCOPE;

    Scene::Component comp = get_ruid_component(ruid);
    if (!comp || !mObj->registry.get_component_world_mat4(comp.cuid(), worldMat4))
        return false;

    return true;
}

Scene::AudioSource::AudioSource(Component comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_AUDIO_SOURCE)
    {
        mData = comp.data();
        mAudioSource = (AudioSourceComponent*)mData;
    }
}

Scene::AudioSource::AudioSource(AudioSourceComponent* comp)
{
    if (comp && comp->base && comp->base->type == COMPONENT_TYPE_AUDIO_SOURCE)
    {
        mData = (ComponentBase**)comp;
        mAudioSource = comp;
    }
}

bool Scene::AudioSource::load(AssetID clipAsset, float pan, float volumeLinear)
{
    return load_audio_source_component(sScene, mAudioSource, clipAsset, pan, volumeLinear);
}

void Scene::AudioSource::play()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    sScene->audioSystemCache.start_playback(mAudioSource->playback);
}

void Scene::AudioSource::pause()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    sScene->audioSystemCache.pause_playback(mAudioSource->playback);
}

void Scene::AudioSource::resume()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    sScene->audioSystemCache.resume_playback(mAudioSource->playback);
}

bool Scene::AudioSource::set_clip_asset(AssetID clipID)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    AudioClipAsset clipA(sScene->assetManager.get_asset(clipID).unwrap());
    AudioBuffer buffer = sScene->audioSystemCache.get_or_create_audio_buffer(clipA);

    if (!buffer)
    {
        mAudioSource->clipID = clipID;
        sScene->audioSystemCache.set_playback_buffer(mAudioSource->playback, buffer);
    }

    return false;
}

AssetID Scene::AudioSource::get_clip_asset()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mAudioSource->clipID;
}

float Scene::AudioSource::get_volume_linear()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mAudioSource->volumeLinear;
}

bool Scene::AudioSource::set_volume_linear(float volume)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    AudioPlayback::Accessor accessor = mAudioSource->playback.access();

    volume = std::clamp(volume, 0.0f, 1.0f);
    mAudioSource->volumeLinear = volume;
    accessor.set_volume_linear(volume);

    return true;
}

float Scene::AudioSource::get_pan()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mAudioSource->pan;
}

bool Scene::AudioSource::set_pan(float pan)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    AudioPlayback::Accessor accessor = mAudioSource->playback.access();

    pan = std::clamp(pan, 0.0f, 1.0f);
    mAudioSource->pan = pan;
    accessor.set_pan(pan);

    return true;
}

Scene::Camera::Camera(Component comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_CAMERA)
    {
        mData = comp.data();
        mCamera = (CameraComponent*)mData;
    }
}

Scene::Camera::Camera(CameraComponent* comp)
{
    if (comp && comp->base && comp->base->cuid)
    {
        mData = (ComponentBase**)comp;
        mCamera = comp;
    }
}

bool Scene::Camera::load_perspective(const CameraPerspectiveInfo& info)
{
    return load_camera_component_perspective(sScene, mCamera, info);
}

bool Scene::Camera::load_orthographic(const CameraOrthographicInfo& info)
{
    return load_camera_component_orthographic(sScene, mCamera, info);
}

bool Scene::Camera::is_main_camera()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mCamera->isMainCamera;
}

bool Scene::Camera::is_perspective()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mCamera->camera.is_perspective();
}

bool Scene::Camera::get_perspective_info(CameraPerspectiveInfo& outInfo)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    if (mCamera->camera.is_perspective())
    {
        outInfo = mCamera->camera.get_perspective();
        return true;
    }

    return false;
}

bool Scene::Camera::get_orthographic_info(CameraOrthographicInfo& outInfo)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    if (!mCamera->camera.is_perspective())
    {
        outInfo = mCamera->camera.get_orthographic();
        return true;
    }

    return false;
}

void Scene::Camera::set_perspective(const CameraPerspectiveInfo& info)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    mCamera->camera.set_perspective(info);
}

void Scene::Camera::set_orthographic(const CameraOrthographicInfo& info)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    mCamera->camera.set_orthographic(info);
}

Scene::Mesh::Mesh(Component comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_MESH)
    {
        mData = comp.data();
        mMesh = (MeshComponent*)mData;
    }
}

Scene::Mesh::Mesh(MeshComponent* comp)
{
    if (comp && comp->base && comp->base->cuid)
    {
        mData = (ComponentBase**)comp;
        mMesh = comp;
    }
}

bool Scene::Mesh::load()
{
    return load_mesh_component(sScene, mMesh, (AssetID)0);
}

bool Scene::Mesh::set_mesh_asset(AssetID meshID)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    MeshData meshData = sScene->renderSystemCache.get_or_create_mesh_data(meshID);

    if (mMesh->draw.set_mesh_asset(meshData))
    {
        mMesh->assetID = meshID;
        return true;
    }

    return false;
}

AssetID Scene::Mesh::get_mesh_asset()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mMesh->assetID;
}

Scene::Sprite2D::Sprite2D(Component comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_SPRITE_2D)
    {
        mData = comp.data();
        mSprite = (Sprite2DComponent*)mData;
    }
}

Scene::Sprite2D::Sprite2D(Sprite2DComponent* comp)
{
    if (comp && comp->base && comp->base->cuid)
    {
        mData = (ComponentBase**)comp;
        mSprite = comp;
    }
}

bool Scene::Sprite2D::load(SUID layerSUID, AssetID textureID)
{
    return load_sprite_2d_component_suid(sScene, mSprite, layerSUID, textureID);
}

bool Scene::Sprite2D::set_texture_2d_asset(AssetID textureID)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    Image2D image = sScene->renderSystemCache.get_or_create_image_2d(textureID);

    if (mSprite->draw.set_image(image))
    {
        mSprite->assetID = textureID;
        return true;
    }

    return false;
}

AssetID Scene::Sprite2D::get_texture_2d_asset()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mSprite->assetID;
}

uint32_t Scene::Sprite2D::get_z_depth()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mSprite->draw.get_z_depth();
}

void Scene::Sprite2D::set_z_depth(uint32_t zDepth)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    mSprite->draw.set_z_depth(zDepth);
}

Vec2 Scene::Sprite2D::get_pivot()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mSprite->draw.get_pivot();
}

void Scene::Sprite2D::set_pivot(const Vec2& pivot)
{
    LD_ASSERT_COMPONENT_LOADED(mData);
}

Rect Scene::Sprite2D::get_region()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mSprite->draw.get_region();
}

void Scene::Sprite2D::set_region(const Rect& rect)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    mSprite->draw.set_region(rect);
}

RUID Scene::Sprite2D::get_screen_layer_ruid()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mSprite->draw.get_layer_id();
}

SUID Scene::Sprite2D::get_screen_layer_suid()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    RUID layerRUID = mSprite->draw.get_layer_id();
    LD_ASSERT(layerRUID);

    SUID layerSUID = sScene->renderSystemCache.get_screen_layer_suid(layerRUID);
    LD_ASSERT(layerSUID);

    return layerSUID;
}

Scene::ScreenUI::ScreenUI(Component comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_SCREEN_UI)
    {
        mData = comp.data();
        mUI = (ScreenUIComponent*)mData;
    }
}

Scene::ScreenUI::ScreenUI(ScreenUIComponent* comp)
{
    if (comp && comp->base && comp->base->cuid)
    {
        mData = (ComponentBase**)comp;
        mUI = comp;
    }
}

bool Scene::ScreenUI::load(AssetID uiTemplateID)
{
    return load_screen_ui_component(sScene, mUI, uiTemplateID);
}

bool Scene::ScreenUI::set_ui_template_asset(AssetID uiTemplateID)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    UITemplateAsset asset = (UITemplateAsset)sScene->assetManager.get_asset(uiTemplateID);
    if (!asset || asset.get_type() != ASSET_TYPE_UI_TEMPLATE)
        return false;

    UILayoutInfo layoutI{};
    UIWindowInfo windowI{};
    UIWorkspace space = sScene->screenUI.workspace();
    mUI->uiWindow = space.create_window(layoutI, windowI, nullptr);

    if (!mUI->uiWindow || !asset.load_ui_subtree(mUI->uiWindow, nullptr, nullptr))
        return false;

    mUI->uiTemplateID = uiTemplateID;

    return true;
}

AssetID Scene::ScreenUI::get_ui_template_asset()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mUI->uiTemplateID;
}

} // namespace LD