#include <Ludens/Camera/Camera.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Math/Math.h>
#include <Ludens/Log/Log.h>
#include <Ludens/Lua/LuaModule.h>
#include <Ludens/Lua/LuaState.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/Scene/Scene.h>
#include <Ludens/System/Timer.h>

// Scene implementation may know about DataRegistry memory layouts.
#include <Ludens/DataRegistry/DataComponent.h>

#include "AudioSystemCache.h"
#include "LuaScript.h"
#include "RenderSystemCache.h"
#include "SceneObj.h"

#include "Component/AudioSourceComponent.h"
#include "Component/Camera2DComponent.h"
#include "Component/CameraComponent.h"
#include "Component/MeshComponent.h"
#include "Component/ScreenUIComponent.h"
#include "Component/Sprite2DComponent.h"

#define SCENE_LOG_CHANNEL_NAME "Scene"

// Scene user's responsibility to check handle before calling methods
#define LD_ASSERT_COMPONENT_LOADED(DATA) LD_ASSERT(DATA && *(DATA) && ((*(DATA))->flags & COMPONENT_FLAG_LOADED_BIT))

namespace LD {

static Log sSceneLog(SCENE_LOG_CHANNEL_NAME);

/// @brief Scene Singleton, all scene operations including transition should be done in-place,
///        the SceneObj address should be immutable.
SceneObj* sScene = nullptr;

/// @brief Component behavior and operations within a Scene.
///        - init: initialize default parameters for creating subsystem resources
///        - clone: copies parameters from an existing component
///        - startup: runtime subsystem resources must be instantiated, script is attached
///        - cleanup: script is detached and strict runtime resources are destroyed
///        - unload: all subsystem resources associated with component must be destroyed
struct SceneComponentMeta
{
    ComponentType type;
    void (*init)(ComponentBase** dstData);
    bool (*clone)(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err);
    bool (*unload)(SceneObj* scene, ComponentBase** data, std::string& err);
    bool (*startup)(SceneObj* scene, ComponentBase** data, std::string& err);
    bool (*cleanup)(SceneObj* scene, ComponentBase** data, std::string& err);
};

// clang-format off
static SceneComponentMeta sSceneComponents[] = {
    {COMPONENT_TYPE_DATA,         nullptr,                      nullptr,                       nullptr,                        nullptr,                      nullptr},
    {COMPONENT_TYPE_AUDIO_SOURCE, &init_audio_source_component, &clone_audio_source_component, &unload_audio_source_component, nullptr,                      &cleanup_audio_source_component},
    {COMPONENT_TYPE_TRANSFORM,    nullptr,                      nullptr,                       nullptr,                        nullptr,                      nullptr},
    {COMPONENT_TYPE_TRANSFORM_2D, nullptr,                      nullptr,                       nullptr,                        nullptr,                      nullptr},
    {COMPONENT_TYPE_CAMERA,       &init_camera_component,       &clone_camera_component,       &unload_camera_component,       &startup_camera_component,    &cleanup_camera_component},
    {COMPONENT_TYPE_CAMERA_2D,    &init_camera_2d_component,    &clone_camera_2d_component,    &unload_camera_2d_component,    &startup_camera_2d_component, &cleanup_camera_2d_component},
    {COMPONENT_TYPE_MESH,         &init_mesh_component,         &clone_mesh_component,         &unload_mesh_component,         nullptr,                      nullptr},
    {COMPONENT_TYPE_SPRITE_2D,    &init_sprite_2d_component,    &clone_sprite_2d_component,    &unload_sprite_2d_component,    nullptr,                      nullptr},
    {COMPONENT_TYPE_SCREEN_UI,    &init_screen_ui_component,    &clone_screen_ui_component,    &unload_screen_ui_component,    &startup_screen_ui_component, &cleanup_screen_ui_component},
};
// clang-format on

static_assert(sizeof(sSceneComponents) / sizeof(*sSceneComponents) == COMPONENT_TYPE_ENUM_COUNT);

//
// IMPLEMENTATION
//

bool SceneObj::load_registry_from_backup()
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(registry && registryBackup);

    Vector<ComponentBase**> dstRoots;
    Vector<ComponentBase**> srcRoots;
    registry.get_root_component_data(dstRoots);
    registryBackup.get_root_component_data(srcRoots);
    LD_ASSERT(dstRoots.size() == srcRoots.size());

    std::string err;

    for (size_t i = 0; i < dstRoots.size(); i++)
    {
        if (!load_subtree_from_backup(dstRoots[i], srcRoots[i], err))
        {
            sSceneLog.error("cleanup complete in {} ms", err);
            return false;
        }
    }

    return true;
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

bool SceneObj::startup_registry()
{
    LD_PROFILE_SCOPE;

    Timer timer;
    timer.start();

    std::string err;
    Vector<ComponentBase**> startupOrder;
    Vector<ComponentBase**> roots;
    registry.get_root_component_data(roots);

    bool success = true;
    std::string errComponentName;

    for (ComponentBase** rootData : roots)
    {
        if (!startup_subtree(rootData, startupOrder, err))
        {
            errComponentName = std::string((*rootData)->name);
            success = false;
            break;
        }
    }

    std::string tmp;
    size_t durationUS = timer.stop();

    if (success)
        sSceneLog.info("startup complete in {} ms", durationUS / 1000.0f);
    else
    {
        for (auto it = startupOrder.rbegin(); it != startupOrder.rend(); ++it)
            (void)cleanup_component(*it, tmp);

        sSceneLog.error("startup failed for component {}:\n{}", errComponentName, err);
    }

    return success;
}

void SceneObj::cleanup_registry()
{
    LD_PROFILE_SCOPE;

    Timer timer;
    timer.start();

    std::string err;
    Vector<ComponentBase**> roots;
    registry.get_root_component_data(roots);

    for (ComponentBase** rootData : roots)
        cleanup_subtree(rootData);

    mainCameraC = nullptr;

    size_t durationUS = timer.stop();

    sSceneLog.info("cleanup complete in {} ms", durationUS / 1000.0f);
}

void SceneObj::resize(const Vec2& newExtent)
{
    if (extent == newExtent)
        return;

    extent = newExtent;
    screenUI.resize(extent);
}

// This is basically the editor loading a copy of component subtree from backup data
bool SceneObj::load_subtree_from_backup(ComponentBase** dstData, ComponentBase** srcData, std::string& err)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(dstData && srcData);

    ComponentBase* dstBase = *dstData;
    ComponentBase* srcBase = *srcData;

    // sanity checks
    LD_ASSERT(srcBase->type == dstBase->type);
    LD_ASSERT(srcBase->suid == dstBase->suid);
    LD_ASSERT((std::string(dstBase->name) == std::string(srcBase->name)));

    // TODO: init should not override Transform...
    sSceneComponents[(int)dstBase->type].init(dstData);
    *dstData = dstBase;

    bool ok = sSceneComponents[(int)dstBase->type].clone(sScene, dstData, srcData, err);
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

        if (!load_subtree_from_backup(dstChildData, srcChildData, err))
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

    std::string err;

    if (sSceneComponents[(int)base->type].unload)
    {
        if (!sSceneComponents[(int)base->type].unload(this, data, err))
            sSceneLog.error("failed to unload component: {}", err);
    }

    for (ComponentBase* child = base->child; child; child = child->next)
    {
        ComponentBase** childData = registry.get_component_data(child->cuid, nullptr);
        unload_subtree(childData);
    }
}

bool SceneObj::startup_subtree(ComponentBase** rootData, Vector<ComponentBase**>& startupOrder, std::string& err)
{
    LD_ASSERT(rootData);
    ComponentBase* rootBase = *rootData;

    for (ComponentBase* childBase = rootBase->child; childBase; childBase = childBase->next)
    {
        ComponentBase** childData = registry.get_component_data(childBase->cuid, nullptr);
        if (!startup_subtree(childData, startupOrder, err))
            return false;
    }

    // post-order traversal, all child components of root already have their scripts attached
    if (!startup_component(rootData, err))
        return false;

    startupOrder.push_back(rootData);

    return true;
}

bool SceneObj::startup_component(ComponentBase** data, std::string& err)
{
    LD_ASSERT(data);
    ComponentBase* base = *data;

    if (sSceneComponents[(int)base->type].startup)
    {
        if (!sSceneComponents[(int)base->type].startup(this, data, err))
            return false;
    }

    std::string tmp;

    if (base->scriptAssetID)
    {
        const CUID rootCUID = base->cuid;
        luaContext.create_component_table(rootCUID);

        if (!luaContext.create_lua_script(rootCUID, base->scriptAssetID, err))
        {
            if (sSceneComponents[(int)base->type].cleanup)
                (void)sSceneComponents[(int)base->type].cleanup(this, data, tmp);

            return false;
        }

        if (!luaContext.attach_lua_script(rootCUID, err))
        {
            luaContext.destroy_lua_script(rootCUID);
            if (sSceneComponents[(int)base->type].cleanup)
                (void)sSceneComponents[(int)base->type].cleanup(this, data, tmp);

            return false;
        }
    }

    return true;
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
    std::string err;
    if (!cleanup_component(rootData, err))
        sSceneLog.error("failed to cleanup component: {}", err);
}

bool SceneObj::cleanup_component(ComponentBase** data, std::string& err)
{
    LD_ASSERT(data);
    ComponentBase* base = *data;

    if (sSceneComponents[(int)base->type].cleanup)
        sSceneComponents[(int)base->type].cleanup(this, data, err);

    bool success = true;

    if (base->scriptAssetID)
    {
        const CUID cuid = base->cuid;
        if (!luaContext.detach_lua_script(cuid, err))
            success = false;

        luaContext.destroy_lua_script(cuid);
        luaContext.destroy_component_table(cuid);
    }

    return success;
}

//
// PUBLIC API
//

const char* get_lua_script_log_channel_name()
{
    return LuaScript::get_log_channel_name();
}

const char* get_scene_log_channel_name()
{
    return SCENE_LOG_CHANNEL_NAME;
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
    sScene->extent = {};

    ScreenUIInfo info{};
    info.extent = {};
    info.fontAtlas = sceneI.fontAtlas;
    info.fontAtlasImage = sceneI.fontAtlasImage;
    info.theme = sceneI.uiTheme;
    LD_ASSERT(info.fontAtlas);
    LD_ASSERT(info.fontAtlasImage);
    LD_ASSERT(info.theme);
    sScene->screenUI = LD::ScreenUI::create(info);

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

    // initial empty state
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

    // force initial UI layout
    mObj->screenUI.update(0.0f);

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

    // create and load a copy for play-in-editor session
    mObj->registryBackup = mObj->registry;
    mObj->registry = mObj->registryBackup.duplicate();
    mObj->load_registry_from_backup();
}

bool Scene::startup()
{
    LD_PROFILE_SCOPE;

    if (mObj->state == SCENE_STATE_RUNNING)
        return true;

    if (mObj->startup_registry())
    {
        mObj->state = SCENE_STATE_RUNNING;
        return true;
    }

    // startup failed

    if (mObj->registryBackup)
    {
        mObj->unload_registry();
        DataRegistry::destroy(mObj->registry);
        mObj->registry = mObj->registryBackup;
        mObj->registryBackup = {};
    }

    return false;
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

    mObj->resize(screenExtent);

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

    for (auto it = mObj->registry.get_components(COMPONENT_TYPE_CAMERA_2D); it; ++it)
    {
        auto* cameraC = (Camera2DComponent*)it.data();
        const ComponentBase* base = cameraC->base;

        Mat4 worldMat4;
        bool ok = mObj->registry.get_component_world_mat4(base->cuid, worldMat4);
        LD_ASSERT(ok);
        Vec2 worldPos(worldMat4[3][0], worldMat4[3][1]);

        float rotRadians = LD_ATAN2(worldMat4[0][1], worldMat4[0][0]);

        cameraC->camera.set_position(worldPos);
        cameraC->camera.set_rotation(LD_TO_DEGREES(rotRadians));
    }

    // any heap allocations for audio is done on main thread.
    mObj->audioSystemCache.update();
}

void Scene::resize(const Vec2& extent)
{
    mObj->resize(extent);
}

void Scene::render_screen_ui(ScreenRenderComponent renderer)
{
    LD_PROFILE_SCOPE;

    mObj->screenUI.render(renderer);
}

void Scene::input_screen_ui(const WindowEvent* event)
{
    LD_PROFILE_SCOPE;

    mObj->screenUI.input(event);
}

Camera Scene::get_camera()
{
    if (mObj->mainCameraC)
        return mObj->mainCameraC->camera;

    return {};
}

Vector<Viewport> Scene::get_screen_regions()
{
    Vector<Viewport> viewports;

    // one Viewport per Camera2DComponent.
    for (auto it = mObj->registry.get_components(COMPONENT_TYPE_CAMERA_2D); it; ++it)
    {
        Camera2DComponent* comp = (Camera2DComponent*)it.data();
        Viewport viewport = comp->camera.get_viewport();
        viewport.region = comp->viewport;
        viewports.push_back(viewport);
    }

    if (!viewports.empty())
        return viewports;

    // if no Camera2DComponents are present, fallback to single fullscreen viewport.
    viewports.push_back(Viewport::from_extent(mObj->extent));
    return viewports;
}

ComponentView Scene::create_component(ComponentType type, const char* name, CUID parentCUID)
{
    CUID compCUID = mObj->registry.create_component(type, name, parentCUID, (SUID)0);

    // TODO: DataRegistry API without CUID -> Component Data chasing.
    ComponentBase** data = mObj->registry.get_component_data(compCUID, nullptr);
    ComponentBase* base = *data;
    sSceneComponents[(int)type].init(data);
    *data = base;

    return ComponentView(data);
}

ComponentView Scene::create_component_serial(ComponentType type, const char* name, SUID parentSUID, SUID hintSUID)
{
    ComponentBase** parentData = mObj->registry.get_component_data_by_suid(parentSUID, nullptr);

    if (parentSUID && !parentData) // bad input, parent does not exist in serial domain
        return {};

    SUID compSUID = hintSUID;

    if (compSUID && !try_get_suid(compSUID)) // already registered
        return {};

    if (!compSUID)
        compSUID = get_suid();

    CUID parentCUID = parentData ? (*parentData)->cuid : 0;
    CUID compCUID = mObj->registry.create_component(type, name, parentCUID, compSUID);

    // TODO: DataRegistry API without CUID -> Component Data chasing.
    ComponentBase** data = mObj->registry.get_component_data(compCUID, nullptr);
    ComponentBase* base = *data;
    sSceneComponents[(int)type].init(data);
    *data = base;

    return ComponentView(data);
}

void Scene::destroy_component(CUID compID)
{
    mObj->registry.destroy_component(compID);
}

void Scene::reparent(CUID compID, CUID parentID)
{
    mObj->registry.reparent(compID, parentID);
}

void Scene::get_root_components(Vector<ComponentView>& roots)
{
    roots.clear();

    Vector<ComponentBase**> rootData;
    mObj->registry.get_root_component_data(rootData);

    // kinda slow for pointer chasing CUID > Component Data
    for (ComponentBase** root : rootData)
    {
        ComponentView comp(root);
        LD_ASSERT(comp);
        roots.push_back(comp);
    }
}

ComponentView Scene::get_component(CUID compID)
{
    return ComponentView(mObj->registry.get_component_data(compID, nullptr));
}

ComponentView Scene::get_component_by_suid(SUID compSUID)
{
    return ComponentView(mObj->registry.get_component_data_by_suid(compSUID, nullptr));
}

ComponentType ComponentView::type()
{
    return (*mData)->type;
}

CUID ComponentView::cuid()
{
    return (*mData)->cuid;
}

SUID ComponentView::suid()
{
    return (*mData)->suid;
}

RUID ComponentView::ruid()
{
    return sScene->renderSystemCache.get_component_draw_id((*mData)->cuid);
}

const char* ComponentView::get_name()
{
    return (*mData)->name;
}

AssetID ComponentView::get_script_asset_id()
{
    return (*mData)->scriptAssetID;
}

void ComponentView::set_script_asset_id(AssetID assetID)
{
    (*mData)->scriptAssetID = assetID;
}

void ComponentView::get_children(Vector<ComponentView>& children)
{
    children.clear();

    // kinda slow with ID -> Data pointer chasing.
    for (ComponentBase* child = (*mData)->child; child; child = child->next)
    {
        ComponentView childC(sScene->registry.get_component_data(child->cuid, nullptr));
        LD_ASSERT(childC);
        children.push_back(childC);
    }
}

ComponentView ComponentView::get_parent()
{
    ComponentBase* parentBase = (*mData)->parent;
    if (!parentBase)
        return {};

    return ComponentView(sScene->registry.get_component_data(parentBase->cuid, nullptr));
}

bool ComponentView::get_transform(TransformEx& transform)
{
    // TODO: DataRegistry API to take ComponentBase* directly
    return sScene->registry.get_component_transform((*mData)->cuid, transform);
}

bool ComponentView::set_transform(const TransformEx& transform)
{
    // TODO: DataRegistry API to take ComponentBase* directly
    return sScene->registry.set_component_transform((*mData)->cuid, transform);
}

bool ComponentView::get_transform_2d(Transform2D& transform)
{
    // TODO: DataRegistry API to take ComponentBase* directly
    return sScene->registry.get_component_transform_2d((*mData)->cuid, transform);
}

bool ComponentView::set_transform_2d(const Transform2D& transform)
{
    // TODO: DataRegistry API to take ComponentBase* directly
    return sScene->registry.set_component_transform_2d((*mData)->cuid, transform);
}

bool ComponentView::get_world_mat4(Mat4& worldMat4)
{
    ComponentBase* base = *mData;

    return sScene->registry.get_component_world_mat4(base->cuid, worldMat4);
}

ComponentView Scene::get_2d_component_by_position(const Vec2& worldPos)
{
    CUID compCUID = sScene->renderSystemCache.get_2d_component_by_position(worldPos, [](RUID ruid, Mat4& mat4, void*) -> bool { return Scene(sScene).get_ruid_world_mat4(ruid, mat4); }, nullptr);

    return ComponentView(sScene->registry.get_component_data(compCUID, nullptr));
}

ComponentView Scene::get_ruid_component(RUID ruid)
{
    CUID compCUID = mObj->renderSystemCache.get_draw_id_component(ruid);

    return ComponentView(sScene->registry.get_component_data(compCUID, nullptr));
}

bool Scene::get_ruid_world_mat4(RUID ruid, Mat4& worldMat4)
{
    LD_PROFILE_SCOPE;

    ComponentView comp = get_ruid_component(ruid);
    if (!comp || !mObj->registry.get_component_world_mat4(comp.cuid(), worldMat4))
        return false;

    return true;
}

} // namespace LD
