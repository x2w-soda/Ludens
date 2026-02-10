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

static bool load_audio_source_component(SceneObj* scene, AudioSourceComponent* source, AssetID clipID);
static void unload_audio_source_component(SceneObj* scene, ComponentBase** source);
static void cleanup_audio_source_component(SceneObj* scene, ComponentBase** source);
static bool load_camera_component_perspective(SceneObj* scene, CameraComponent* camera, const CameraPerspectiveInfo& perspectiveI);
static bool load_camera_component_orthographic(SceneObj* scene, CameraComponent* camera, const CameraOrthographicInfo& perspectiveI);
static void unload_camera_component(SceneObj* scene, ComponentBase** camera);
static void startup_camera_component(SceneObj* scene, ComponentBase** data);
static void cleanup_camera_component(SceneObj* scene, ComponentBase** data);
static bool load_mesh_component(SceneObj* scene, MeshComponent* mesh);
static void unload_mesh_component(SceneObj* scene, ComponentBase** data);
static bool load_sprite_2d_component(SceneObj* scene, Sprite2DComponent* sprite, SUID screenLayerID);
static void unload_sprite_2d_component(SceneObj* scene, ComponentBase** data);

/// @brief Component behavior and operations within a Scene.
///        - loading a component creates subsystem resources
///        - startup a component to prepare it for runtime
///        - cleanup a component to release runtime resources
///        - unloading a component destroys subsystem resources
struct SceneComponent
{
    ComponentType type;
    void (*unload)(SceneObj* scene, ComponentBase** data);
    void (*startup)(SceneObj* scene, ComponentBase** data);
    void (*cleanup)(SceneObj* scene, ComponentBase** data);
};

// clang-format off
static SceneComponent sSceneComponents[] = {
    {COMPONENT_TYPE_DATA,         nullptr,                        nullptr,                    nullptr},
    {COMPONENT_TYPE_AUDIO_SOURCE, &unload_audio_source_component, nullptr,                    &cleanup_audio_source_component},
    {COMPONENT_TYPE_TRANSFORM,    nullptr,                        nullptr,                    nullptr},
    {COMPONENT_TYPE_CAMERA,       &unload_camera_component,       &startup_camera_component,  &cleanup_camera_component},
    {COMPONENT_TYPE_MESH,         &unload_mesh_component,         nullptr,                    nullptr},
    {COMPONENT_TYPE_SPRITE_2D,    &unload_sprite_2d_component,    nullptr,                    nullptr},
};
// clang-format on

static_assert(sizeof(sSceneComponents) / sizeof(*sSceneComponents) == COMPONENT_TYPE_ENUM_COUNT);

//
// IMPLEMENTATION
//

static bool load_audio_source_component(SceneObj* scene, AudioSourceComponent* source, AssetID clipID)
{
    // NOTE: Buffer not destroyed upon component unload.
    //       Other components may still be using it for playback.
    AudioBuffer buffer = scene->audioSystemCache.get_or_create_audio_buffer(clipID);
    if (!buffer)
        return false;

    source->playback = scene->audioSystemCache.create_playback(buffer, source);
    if (!source->playback)
        return false;

    source->clipID = clipID;
    source->base->flags |= COMPONENT_FLAG_LOADED_BIT;
    return true;
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
    camera->camera = LD::Camera::create(perspectiveI, Vec3(0.0f));

    if (!camera->camera)
        return false;

    camera->base->flags |= COMPONENT_FLAG_LOADED_BIT;
    return true;
}

static bool load_camera_component_orthographic(SceneObj* scene, CameraComponent* camera, const CameraOrthographicInfo& perspectiveI)
{
    camera->camera = LD::Camera::create(perspectiveI, Vec3(0.0f));

    if (!camera->camera)
        return false;

    camera->base->flags |= COMPONENT_FLAG_LOADED_BIT;
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

static bool load_mesh_component(SceneObj* scene, MeshComponent* mesh)
{
    ComponentBase* base = mesh->base;

    mesh->draw = scene->renderSystemCache.create_mesh_draw(base->cuid, 0);

    if (!mesh->draw)
        return false;

    base->flags |= COMPONENT_FLAG_LOADED_BIT;
    return true;
}

void unload_mesh_component(SceneObj* scene, ComponentBase** data)
{
    MeshComponent* mesh = (MeshComponent*)data;
    ComponentBase* base = mesh->base;

    if (mesh->draw)
    {
        scene->renderSystemCache.destroy_mesh_draw(mesh->draw);
        mesh->draw = {};
    }

    base->flags &= ~COMPONENT_FLAG_LOADED_BIT;
}

static bool load_sprite_2d_component(SceneObj* scene, Sprite2DComponent* sprite, SUID screenLayerID)
{
    ComponentBase* base = sprite->base;

    RUID layerRUID = scene->renderSystemCache.get_or_create_screen_layer(screenLayerID);
    if (!layerRUID)
        return false;

    sprite->draw = scene->renderSystemCache.create_sprite_2d_draw(base->cuid, layerRUID, 0);
    if (!sprite->draw)
        return false;

    base->flags |= COMPONENT_FLAG_LOADED_BIT;
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

void SceneObj::unload_subtree(ComponentBase** data)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(data);
    ComponentBase* base = (*data);

    if (sSceneComponents[(int)base->type].unload && (base->flags & COMPONENT_FLAG_LOADED_BIT))
        sSceneComponents[(int)base->type].unload(this, data);

    // sanity check
    LD_ASSERT((base->flags & COMPONENT_FLAG_LOADED_BIT) == 0);

    for (ComponentBase* child = base->child; child; child = child->next)
    {
        ComponentBase** childData = registry.get_component_data(child->cuid, nullptr);
        unload_subtree(childData);
    }
}

void SceneObj::startup_subtree(CUID rootID)
{
    ComponentBase* rootBase = registry.get_component_base(rootID);
    LD_ASSERT(rootBase->flags & COMPONENT_FLAG_LOADED_BIT);

    if (!rootBase)
        return;

    for (ComponentBase* child = rootBase->child; child; child = child->next)
    {
        startup_subtree(child->cuid);
    }

    // post-order traversal, all child components of root already have their scripts attached
    ComponentType type;
    ComponentBase** rootData = registry.get_component_data(rootBase->cuid, &type);
    LD_ASSERT(type == rootBase->type);

    if (sSceneComponents[(int)type].startup)
        sSceneComponents[(int)type].startup(this, rootData);

    if (rootBase->scriptAssetID)
    {
        luaContext.create_component_table(rootBase->cuid);
        bool success = luaContext.create_lua_script(rootID, rootBase->scriptAssetID); // TODO: abort startup at the first failure of creating lua script instance.
        LD_ASSERT(success);
        luaContext.attach_lua_script(rootID);
    }
}

void SceneObj::cleanup_subtree(CUID rootID)
{
    ComponentBase* rootC = registry.get_component_base(rootID);

    if (!rootC)
        return;

    for (ComponentBase* childC = rootC->child; childC; childC = childC->next)
    {
        cleanup_subtree(childC->cuid);
    }

    // post-order traversal, all child components of root already have their scripts detached
    ComponentType type;
    ComponentBase** compData = registry.get_component_data(rootC->cuid, &type);
    LD_ASSERT(type == rootC->type);

    if (sSceneComponents[(int)type].cleanup)
        sSceneComponents[(int)type].cleanup(this, compData);

    luaContext.detach_lua_script(rootID);
    luaContext.destroy_lua_script(rootID);

    // component table may exist even if there is no script attached it
    luaContext.destroy_component_table(rootC->cuid);
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
    sScene->luaContext.create(Scene(sScene), sScene->registry, sceneI.assetManager);

    return Scene(sScene);
}

void Scene::destroy(Scene scene)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(sScene && sScene == scene.unwrap());

    // destroy all components
    scene.reset();

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

    if (mObj->registryBack)
    {
        DataRegistry::destroy(mObj->registryBack);
        mObj->registryBack = {};
    }

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

    Vector<CUID> roots;
    mObj->registry.get_root_components(roots);
}

void Scene::unload()
{
    LD_PROFILE_SCOPE;

    if (mObj->state == SCENE_STATE_EMPTY)
        return;

    Vector<CUID> roots;
    mObj->registry.get_root_components(roots);

    for (CUID rootID : roots)
    {
        ComponentBase** rootData = mObj->registry.get_component_data(rootID, nullptr);
        mObj->unload_subtree(rootData);
    }

    mObj->state = SCENE_STATE_EMPTY;
}

void Scene::startup()
{
    LD_PROFILE_SCOPE;

    if (mObj->state == SCENE_STATE_RUNNING)
        return;

    mObj->state = SCENE_STATE_RUNNING;
    mObj->luaContext.set_registry(mObj->registry);

    std::vector<CUID> roots;
    mObj->registry.get_root_components(roots);

    for (CUID root : roots)
    {
        mObj->startup_subtree(root);
    }
}

void Scene::cleanup()
{
    LD_PROFILE_SCOPE;

    if (mObj->state != SCENE_STATE_RUNNING)
        return;

    mObj->state = SCENE_STATE_LOADED;

    Vector<CUID> roots;
    mObj->registry.get_root_components(roots);

    for (CUID root : roots)
    {
        mObj->cleanup_subtree(root);
    }

    mObj->mainCameraC = nullptr;
}

void Scene::backup()
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(mObj->state == SCENE_STATE_LOADED);

    if (mObj->registryBack)
        DataRegistry::destroy(mObj->registryBack);

    mObj->registryBack = mObj->registry.duplicate();
}

void Scene::swap()
{
    LD_ASSERT(mObj->state == SCENE_STATE_LOADED);

    DataRegistry tmp = mObj->registry;
    mObj->registry = mObj->registryBack;
    mObj->registryBack = tmp;
}

void Scene::update(const Vec2& screenExtent, float delta)
{
    LD_PROFILE_SCOPE;
    LD_ASSERT(screenExtent.x > 0.0f && screenExtent.y > 0.0f);

    mObj->screenExtent = screenExtent;

    // update all lua script instances
    mObj->luaContext.update(delta);

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

void Scene::get_root_component_cuids(Vector<CUID>& roots)
{
    mObj->registry.get_root_components(roots);
}

void Scene::get_root_components(Vector<Component>& roots)
{
    roots.clear();

    Vector<CUID> rootCUIDs;
    mObj->registry.get_root_components(rootCUIDs);

    // kinda slow for pointer chasing CUID > Component Data
    for (CUID rootCUID : rootCUIDs)
    {
        Scene::Component comp(mObj->registry.get_component_data(rootCUID, nullptr));
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

Mat4 Scene::get_ruid_transform_mat4(RUID ruid)
{
    LD_PROFILE_SCOPE;

    Scene::Component comp = get_ruid_component(ruid);
    if (!comp)
        return Mat4(1.0f);

    Mat4 worldMat4;
    if (!mObj->registry.get_component_world_mat4(comp.cuid(), worldMat4))
        return Mat4(1.0f);

    return worldMat4;
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

bool Scene::AudioSource::load(AssetID clipAsset)
{
    return load_audio_source_component(sScene, mAudioSource, clipAsset);
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
    return load_mesh_component(sScene, mMesh);
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

bool Scene::Sprite2D::load(SUID layerSUID)
{
    return load_sprite_2d_component(sScene, mSprite, layerSUID);
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

Rect Scene::Sprite2D::get_rect()
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    return mSprite->draw.get_rect();
}

void Scene::Sprite2D::set_rect(const Rect& rect)
{
    LD_ASSERT_COMPONENT_LOADED(mData);

    mSprite->draw.set_rect(rect);
}

} // namespace LD