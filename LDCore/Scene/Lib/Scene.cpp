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
#include "Component/Transform2DComponent.h"

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
    const PropertyMetaTable* propMetaTable;
    void (*init)(ComponentBase** dstData);
    bool (*clone)(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, std::string& err);
    bool (*load)(SceneObj* scene, ComponentBase** data, const Vector<PropertyValue>& props, std::string& err);
    bool (*unload)(SceneObj* scene, ComponentBase** data, std::string& err);
    bool (*startup)(SceneObj* scene, ComponentBase** data, std::string& err);
    bool (*cleanup)(SceneObj* scene, ComponentBase** data, std::string& err);
    AssetID (*getAsset)(SceneObj* scene, ComponentBase** data, uint32_t assetSlotIndex);
    bool (*setAsset)(SceneObj* scene, ComponentBase** data, uint32_t assetSlotIndex, AssetID assetID);
    AssetType (*getAssetType)(SceneObj* scene, uint32_t assetSlotIndex);
};

static void init_nop(ComponentBase**) {}
static bool clone_nop(SceneObj*, ComponentBase**, ComponentBase**, std::string&) { return true; }

// clang-format off
static SceneComponentMeta sSceneComponents[] = {
    {COMPONENT_TYPE_DATA,         nullptr,                    nullptr,                      nullptr,                       nullptr,                           nullptr,                        nullptr,                      nullptr,                         nullptr, nullptr},
    {COMPONENT_TYPE_AUDIO_SOURCE, &gAudioSourcePropMetaTable, &AudioSourceMeta::init,       &AudioSourceMeta::clone,       &AudioSourceMeta::load_from_props, &AudioSourceMeta::unload,       nullptr,                      &AudioSourceMeta::cleanup,       &AudioSourceMeta::get_asset, &AudioSourceMeta::set_asset, &AudioSourceMeta::get_asset_type},
    {COMPONENT_TYPE_TRANSFORM,    nullptr,                    &init_nop,                    &clone_nop,                    nullptr,                           nullptr,                        nullptr,                      nullptr,                         nullptr, nullptr},
    {COMPONENT_TYPE_TRANSFORM_2D, &gTransform2DPropMetaTable, &Transform2DMeta::init,       &Transform2DMeta::clone,       &Transform2DMeta::load_from_props, nullptr,                        nullptr,                      nullptr,                         nullptr, nullptr},
    {COMPONENT_TYPE_CAMERA,       nullptr,                    &init_camera_component,       &clone_camera_component,       nullptr,                           &unload_camera_component,       &startup_camera_component,    &cleanup_camera_component,       nullptr, nullptr},
    {COMPONENT_TYPE_CAMERA_2D,    &gCamera2DPropMetaTable,    &Camera2DMeta::init,          &Camera2DMeta::clone,          &Camera2DMeta::load_from_props,    &Camera2DMeta::unload,          &Camera2DMeta::startup,       &Camera2DMeta::cleanup,          nullptr, nullptr},
    {COMPONENT_TYPE_MESH,         nullptr,                    &init_mesh_component,         &clone_mesh_component,         nullptr,                           &unload_mesh_component,         nullptr,                      nullptr,                         nullptr, nullptr},
    {COMPONENT_TYPE_SPRITE_2D,    &gSprite2DPropMetaTable,    &Sprite2DMeta::init,          &Sprite2DMeta::clone,          &Sprite2DMeta::load_from_props,    &Sprite2DMeta::unload,          &Sprite2DMeta::startup,       &Sprite2DMeta::cleanup,          &Sprite2DMeta::get_asset, &Sprite2DMeta::set_asset, &Sprite2DMeta::get_asset_type},
    {COMPONENT_TYPE_SCREEN_UI,    nullptr,                    &init_screen_ui_component,    &clone_screen_ui_component,    nullptr,                           &unload_screen_ui_component,    &startup_screen_ui_component, &cleanup_screen_ui_component,    nullptr, nullptr},
};
// clang-format on

static_assert(sizeof(sSceneComponents) / sizeof(*sSceneComponents) == COMPONENT_TYPE_ENUM_COUNT);

static AssetID scene_component_get_asset(SceneObj* scene, ComponentBase** data, uint32_t assetSlotIndex)
{
    ComponentBase* base = *data;

    if (sSceneComponents[(int)base->type].getAsset)
        return sSceneComponents[(int)base->type].getAsset(scene, data, assetSlotIndex);

    return AssetID(0);
}

static bool scene_component_set_asset(SceneObj* scene, ComponentBase** data, uint32_t assetSlotIndex, AssetID assetID)
{
    ComponentBase* base = *data;

    if (sSceneComponents[(int)base->type].setAsset)
        return sSceneComponents[(int)base->type].setAsset(scene, data, assetSlotIndex, assetID);

    return false;
}

static AssetType scene_component_get_asset_type(SceneObj* scene, ComponentType type, uint32_t assetSlotIndex)
{
    if (sSceneComponents[(int)type].getAssetType)
        return sSceneComponents[(int)type].getAssetType(scene, assetSlotIndex);

    return ASSET_TYPE_ENUM_COUNT;
}

//
// IMPLEMENTATION
//

SceneContext::SceneContext(const SceneContextInfo& info)
{
    LD_PROFILE_SCOPE;

    registry = DataRegistry::create();
    lua.create(Scene(sScene));

    ScreenUIInfo uiI{};
    uiI.extent = {};
    uiI.font = info.uiFont;
    uiI.theme = info.uiTheme;
    LD_ASSERT(uiI.font);
    LD_ASSERT(uiI.theme);
    screenUI = LD::ScreenUI::create(uiI);
}

SceneContext::~SceneContext()
{
    LD_PROFILE_SCOPE;

    ScreenUI::destroy(screenUI);
    lua.destroy();
    DataRegistry::destroy(registry);
}

void SceneContext::invalidate(Vec2 extent)
{
    registry.invalidate_transforms();
    invalidate_cameras(extent);

    SceneUpdateTick tick{};
    tick.extent = extent;
    screenUI.update(tick);
}

void SceneContext::invalidate_cameras(Vec2 extent)
{
    for (auto it = registry.get_components(COMPONENT_TYPE_CAMERA_2D); it; ++it)
    {
        auto* cameraC = (Camera2DComponent*)it.data();
        const ComponentBase* base = cameraC->base;

        Mat4 worldMat4;
        bool ok = registry.get_component_world_mat4(base->cuid, worldMat4);
        LD_ASSERT(ok);
        Vec2 worldPos(worldMat4[3][0], worldMat4[3][1]);

        float rotRadians = LD_ATAN2(worldMat4[0][1], worldMat4[0][0]);

        cameraC->camera.set_position(worldPos);
        cameraC->camera.set_rotation(LD_TO_DEGREES(rotRadians));

        switch (cameraC->constraint)
        {
        case CAMERA_2D_CONSTRAINT_FIXED:
        {
            float width = cameraC->viewport.w * extent.x;
            float height = cameraC->viewport.h * extent.y;
            cameraC->camera.set_extent(Vec2(width, height));
            break;
        }
        case CAMERA_2D_CONSTRAINT_FREE:
        default:
            break;
        }
    }
}

void SceneContext::update(const SceneUpdateTick& tick)
{
    LD_PROFILE_SCOPE;

    // update all lua script instances
    std::string err;
    bool success = lua.update(tick.delta, err);
    if (!success)
        sSceneLog.error("script update failed: {}", err);

    // after this, the transforms are thread-safe read-only for the rest of the frame
    registry.invalidate_transforms();

    invalidate_cameras(tick.extent);

    // update screen space UI
    screenUI.update(tick);
}

bool SceneContext::startup_registry()
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

void SceneContext::cleanup_registry()
{
    LD_PROFILE_SCOPE;

    Timer timer;
    timer.start();

    std::string err;
    Vector<ComponentBase**> roots;
    registry.get_root_component_data(roots);

    for (ComponentBase** rootData : roots)
        cleanup_subtree(rootData);

    size_t durationUS = timer.stop();

    sSceneLog.info("cleanup complete in {} ms", durationUS / 1000.0f);
}

bool SceneContext::startup_subtree(ComponentBase** rootData, Vector<ComponentBase**>& startupOrder, std::string& err)
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

bool SceneContext::startup_component(ComponentBase** data, std::string& err)
{
    LD_ASSERT(data);
    ComponentBase* base = *data;

    if (sSceneComponents[(int)base->type].startup)
    {
        if (!sSceneComponents[(int)base->type].startup(sScene, data, err))
            return false;
    }

    std::string tmp;

    if (base->scriptAssetID)
    {
        const CUID rootCUID = base->cuid;
        lua.create_component_table(rootCUID);

        if (!lua.create_lua_script(rootCUID, base->scriptAssetID, err))
        {
            if (sSceneComponents[(int)base->type].cleanup)
                (void)sSceneComponents[(int)base->type].cleanup(sScene, data, tmp);

            return false;
        }

        if (!lua.attach_lua_script(rootCUID, err))
        {
            lua.destroy_lua_script(rootCUID);
            if (sSceneComponents[(int)base->type].cleanup)
                (void)sSceneComponents[(int)base->type].cleanup(sScene, data, tmp);

            return false;
        }
    }

    return true;
}

void SceneContext::cleanup_subtree(ComponentBase** rootData)
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

bool SceneContext::cleanup_component(ComponentBase** data, std::string& err)
{
    LD_ASSERT(data);
    ComponentBase* base = *data;

    if (sSceneComponents[(int)base->type].cleanup)
        sSceneComponents[(int)base->type].cleanup(sScene, data, err);

    bool success = true;

    if (base->scriptAssetID)
    {
        const CUID cuid = base->cuid;
        if (!lua.detach_lua_script(cuid, err))
            success = false;

        lua.destroy_lua_script(cuid);
        lua.destroy_component_table(cuid);
    }

    return success;
}

void SceneContext::unload_registry(SUIDRegistry suidRegistry)
{
    LD_PROFILE_SCOPE;

    Vector<ComponentBase**> roots;
    registry.get_root_component_data(roots);

    for (ComponentBase** rootData : roots)
    {
        unload_subtree(rootData, suidRegistry);
    }
}

void SceneContext::unload_subtree(ComponentBase** data, SUIDRegistry suidRegistry)
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(suidRegistry);
    LD_ASSERT(data);
    ComponentBase* base = (*data);

    std::string err;

    if (sSceneComponents[(int)base->type].unload)
    {
        if (!sSceneComponents[(int)base->type].unload(sScene, data, err))
            sSceneLog.error("failed to unload component: {}", err);
    }

    if (base->suid)
        suidRegistry.free_suid(base->suid);

    for (ComponentBase* child = base->child; child; child = child->next)
    {
        ComponentBase** childData = registry.get_component_data(child->cuid, nullptr);
        unload_subtree(childData, suidRegistry);
    }
}

bool SceneObj::load_registry_from_backup()
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(active && backup);

    Vector<ComponentBase**> dstRoots;
    Vector<ComponentBase**> srcRoots;
    active->registry.get_root_component_data(dstRoots);
    backup->registry.get_root_component_data(srcRoots);
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

// Given two subtrees with identical hierarchy, create subsystem handles for dst subtree
bool SceneObj::clone_subtree(ComponentBase** dstData, ComponentBase** srcData, std::string& err)
{
    ComponentBase* dstBase = *dstData;
    ComponentBase* srcBase = *srcData;

    // sanity checks
    LD_ASSERT(srcBase->type == dstBase->type);
    LD_ASSERT(srcBase->suid != dstBase->suid);

    if (sSceneComponents[(int)dstBase->type].init)
        sSceneComponents[(int)dstBase->type].init(dstData);
    *dstData = dstBase;

    if (!sSceneComponents[(int)dstBase->type].clone(sScene, dstData, srcData, err))
        return false;

    ComponentBase* dstChild = dstBase->child;
    ComponentBase* srcChild = srcBase->child;

    while (dstChild)
    {
        LD_ASSERT(srcChild);
        ComponentBase** dstChildData = active->registry.get_component_data(dstChild->cuid, nullptr);
        ComponentBase** srcChildData = active->registry.get_component_data(srcChild->cuid, nullptr);

        if (!clone_subtree(dstChildData, srcChildData, err))
            return false;

        dstChild = dstChild->next;
        srcChild = srcChild->next;
    }

    return true;
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

    sSceneComponents[(int)dstBase->type].init(dstData);
    *dstData = dstBase;

    if (!sSceneComponents[(int)dstBase->type].clone(sScene, dstData, srcData, err))
        return false;

    ComponentBase* dstChild = dstBase->child;
    ComponentBase* srcChild = srcBase->child;

    while (dstChild)
    {
        LD_ASSERT(srcChild);
        ComponentBase** dstChildData = active->registry.get_component_data(dstChild->cuid, nullptr);
        ComponentBase** srcChildData = backup->registry.get_component_data(srcChild->cuid, nullptr);

        if (!load_subtree_from_backup(dstChildData, srcChildData, err))
            return false;

        dstChild = dstChild->next;
        srcChild = srcChild->next;
    }

    return true;
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
    LD_ASSERT(sceneI.suidRegistry);

    sScene = heap_new<SceneObj>(MEMORY_USAGE_SCENE);
    sScene->renderSystemCache.create(sceneI.renderSystem);
    sScene->audioSystemCache.create(sceneI.audioSystem);
    sScene->suidRegistry = sceneI.suidRegistry;

    sScene->contextInfo.uiFont = sceneI.uiFont;
    sScene->contextInfo.uiTheme = sceneI.uiTheme;
    sScene->active = nullptr;

    return Scene(sScene);
}

void Scene::destroy()
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(sScene);
    LD_ASSERT(sScene->backup == nullptr);
    LD_ASSERT(sScene->shadow == nullptr);

    // destroy all components
    Scene(sScene).reset();

    if (sScene->active)
        heap_delete<SceneContext>(sScene->active);

    sScene->suidRegistry = {};
    sScene->audioSystemCache.destroy();
    sScene->renderSystemCache.destroy();

    heap_delete<SceneObj>(sScene);
    sScene = nullptr;
}

Scene Scene::get()
{
    return Scene(sScene);
}

void Scene::reset()
{
    LD_PROFILE_SCOPE;

    if (mObj->state == SCENE_STATE_LOADED)
        unload();

    LD_ASSERT(mObj->state == SCENE_STATE_EMPTY);
    LD_ASSERT(!mObj->backup);
}

void Scene::load(const SceneLoadFn& loadFn)
{
    LD_PROFILE_SCOPE;

    if (mObj->state == SCENE_STATE_LOADED)
    {
        LD_UNREACHABLE;
        return;
    }

    LD_ASSERT(!mObj->active);
    mObj->active = heap_new<SceneContext>(MEMORY_USAGE_SCENE, mObj->contextInfo);

    bool success = loadFn(*this);
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

    mObj->active->unload_registry(mObj->suidRegistry);
    heap_delete<SceneContext>(mObj->active);
    mObj->active = nullptr;

    mObj->state = SCENE_STATE_EMPTY;
}

void Scene::backup()
{
    LD_PROFILE_SCOPE;

    LD_ASSERT(mObj->state == SCENE_STATE_LOADED);

    if (mObj->backup)
        heap_delete<SceneContext>(mObj->backup);

    // create and load a copy for play-in-editor session
    mObj->backup = mObj->active;
    mObj->active = heap_new<SceneContext>(MEMORY_USAGE_SCENE, mObj->contextInfo);
    if (mObj->active->registry)
        DataRegistry::destroy(mObj->active->registry);
    mObj->active->registry = mObj->backup->registry.duplicate();
    mObj->load_registry_from_backup();
}

bool Scene::startup()
{
    LD_PROFILE_SCOPE;

    if (mObj->state == SCENE_STATE_RUNNING)
        return true;

    if (mObj->active->startup_registry())
    {
        mObj->state = SCENE_STATE_RUNNING;
        return true;
    }

    // startup failed

    if (mObj->backup)
    {
        mObj->active->unload_registry(mObj->suidRegistry);
        heap_delete<SceneContext>(mObj->active);
        mObj->active = mObj->backup;
        mObj->backup = nullptr;
    }

    return false;
}

void Scene::cleanup()
{
    LD_PROFILE_SCOPE;

    if (mObj->state != SCENE_STATE_RUNNING)
        return;

    mObj->state = SCENE_STATE_LOADED;

    mObj->active->cleanup_registry();

    // after play-in-editor session, restore the backup.
    if (mObj->backup)
    {
        mObj->active->unload_registry(mObj->suidRegistry);
        heap_delete<SceneContext>(mObj->active);

        mObj->active = mObj->backup;
        mObj->backup = nullptr;
    }
}

void Scene::update(const SceneUpdateTick& tick)
{
    LD_PROFILE_SCOPE;
    LD_ASSERT(tick.extent.x > 0.0f && tick.extent.y > 0.0f);

    mObj->tick = tick;

    if (mObj->transition.inProgress)
    {
        // NOTE: This is eventually where we do async I/O to load assets
        //       for next Scene and populate the shadow SceneContext.
        //       For now this is just a dumb synchronous call that blocks
        //       until the full transition is complete.

        mObj->shadow = heap_new<SceneContext>(MEMORY_USAGE_SCENE, mObj->contextInfo);

        bool loadSuccess;

        {
            LD_PROFILE_SCOPE_NAME("Scene ShadowContext Load");

            mObj->contextTarget = SCENE_CONTEXT_SHADOW;

            loadSuccess = mObj->transition.loadFn(sScene);
            mObj->contextTarget = SCENE_CONTEXT_ACTIVE;
        }

        if (loadSuccess)
        {
            if (mObj->shadow->startup_registry())
            {
                mObj->active->cleanup_registry();
                heap_delete<SceneContext>(mObj->active);
                mObj->active = mObj->shadow;
            }
            else
            {
                mObj->shadow->unload_registry(mObj->suidRegistry);
                heap_delete<SceneContext>(mObj->shadow);
                sSceneLog.error("failed to transition to scene, startup failed");
            }
        }
        else
        {
            heap_delete<SceneContext>(mObj->shadow);
            sSceneLog.error("failed to transition to scene, load failed");
        }

        mObj->shadow = nullptr;
        mObj->transition.inProgress = false;
    }

    mObj->active->update(mObj->tick);

    // any heap allocations for audio is done on main thread.
    mObj->audioSystemCache.update();
}

bool Scene::request_transition(const SceneLoadFn& loadFn)
{
    if (mObj->transition.inProgress)
        return false;

    mObj->transition.loadFn = loadFn;
    mObj->transition.inProgress = true;

    return mObj->transition.inProgress;
}

void Scene::render_screen_ui(ScreenRenderComponent renderer)
{
    LD_PROFILE_SCOPE;

    mObj->active->screenUI.render(renderer);
}

void Scene::input_screen_ui(const WindowEvent* event)
{
    LD_PROFILE_SCOPE;

    mObj->active->screenUI.input(event);
}

Camera Scene::get_camera()
{
    return {};
}

void Scene::get_screen_regions(Vector<Viewport>& outViewports, Vector<Rect>& outWorldAABBs)
{
    // one Viewport per Camera2DComponent.
    for (auto it = mObj->active->registry.get_components(COMPONENT_TYPE_CAMERA_2D); it; ++it)
    {
        Camera2DComponent* comp = (Camera2DComponent*)it.data();
        Viewport viewport = comp->camera.get_viewport();
        viewport.region = comp->viewport;
        outViewports.push_back(viewport);
        outWorldAABBs.push_back(comp->camera.get_world_aabb());
    }

    if (!outViewports.empty())
        return;

    // if no Camera2DComponents are present, fallback to single fullscreen viewport.
    Vec2 extent = mObj->tick.extent;
    outViewports.push_back(Viewport::from_extent(extent));
    outWorldAABBs.push_back(Rect(0.0f, 0.0f, extent.x, extent.y));
}

SceneRenderSystem Scene::render_system()
{
    LD_ASSERT(mObj);

    return SceneRenderSystem(mObj);
}

ComponentView Scene::create_component(ComponentType type, const char* name, CUID parentCUID)
{
    DataRegistry reg{};
    switch (mObj->contextTarget)
    {
    case SCENE_CONTEXT_ACTIVE:
        reg = mObj->active->registry;
        break;
    case SCENE_CONTEXT_SHADOW:
        reg = mObj->shadow->registry;
        break;
    }

    CUID compCUID = reg.create_component(type, name, parentCUID, (SUID)0);

    // TODO: DataRegistry API without CUID -> Component Data chasing.
    ComponentBase** data = reg.get_component_data(compCUID, nullptr);
    ComponentBase* base = *data;
    sSceneComponents[(int)type].init(data);
    *data = base;

    return ComponentView(data);
}

ComponentView Scene::create_component_serial(ComponentType type, const char* name, SUIDRegistry suidRegistry, SUID parentSUID, SUID hintSUID)
{
    ComponentBase** parentData = mObj->active->registry.get_component_data_by_suid(parentSUID, nullptr);

    if (parentSUID && !parentData) // bad input, parent does not exist in serial domain
        return {};

    SUID compSUID = hintSUID;

    if (compSUID && !suidRegistry.try_get_suid(compSUID))
        return {}; // already registered, reject bad input

    if (!compSUID)
        compSUID = suidRegistry.get_suid(SERIAL_TYPE_COMPONENT);

    CUID parentCUID = parentData ? (*parentData)->cuid : (CUID)0;
    CUID compCUID = mObj->active->registry.create_component(type, name, parentCUID, compSUID);

    // TODO: DataRegistry API without CUID -> Component Data chasing.
    ComponentBase** data = mObj->active->registry.get_component_data(compCUID, nullptr);
    ComponentBase* base = *data;
    sSceneComponents[(int)type].init(data);
    *data = base;

    return ComponentView(data);
}

void Scene::destroy_component_subtree(CUID compID, SUIDRegistry suidRegistry)
{
    LD_ASSERT(mObj->state != SCENE_STATE_RUNNING);

    ComponentBase** compData = mObj->active->registry.get_component_data(compID, nullptr);
    if (!compData)
        return;

    mObj->active->unload_subtree(compData, suidRegistry);

    mObj->active->registry.destroy_component_subtree(compID);
}

void Scene::reparent_component_subtree(CUID compID, CUID parentID)
{
    LD_ASSERT(mObj->state != SCENE_STATE_RUNNING);

    mObj->active->registry.reparent_component_subtree(compID, parentID);
}

ComponentView Scene::clone_component_subtree(CUID rootID, SUIDRegistry suidRegistry)
{
    ComponentBase** dstData = mObj->active->registry.clone_component_subtree(rootID, suidRegistry);
    if (!dstData)
        return {};

    // Data registry only clones ComponentBase members such as IDs and transform.
    // At the Scene level we clone the component data and create subsystem handles.
    ComponentBase** srcData = mObj->active->registry.get_component_data(rootID, nullptr);
    LD_ASSERT(srcData);

    std::string err;
    if (!mObj->clone_subtree(dstData, srcData, err))
    {
        sSceneLog.error("failed to clone component subtree {}", err);
        return {};
    }

    return ComponentView(dstData);
}

void Scene::get_root_components(Vector<ComponentView>& roots)
{
    roots.clear();

    Vector<ComponentBase**> rootData;
    mObj->active->registry.get_root_component_data(rootData);

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
    return ComponentView(mObj->active->registry.get_component_data(compID, nullptr));
}

Vector<ComponentView> Scene::get_components(ComponentType type)
{
    Vector<ComponentView> views; // TODO: reserve size...

    for (auto it = mObj->active->registry.get_components(type); it; ++it)
        views.push_back(ComponentView((ComponentBase**)it.data()));

    return views;
}

ComponentView Scene::get_component_by_suid(SUID compSUID)
{
    LD_ASSERT(compSUID && compSUID.type() == SERIAL_TYPE_COMPONENT);

    return ComponentView(mObj->active->registry.get_component_data_by_suid(compSUID, nullptr));
}

ComponentView Scene::get_component_by_path(const Vector<int>& path)
{
    return ComponentView(mObj->active->registry.get_component_data_by_path(path));
}

const PropertyMetaTable* ComponentView::property_meta_table()
{
    const PropertyMetaTable* table = sSceneComponents[(int)type()].propMetaTable;

    LD_ASSERT(table);
    return table;
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

bool ComponentView::load_from_props(const Vector<PropertyValue>& props, std::string& err)
{
    int type = (*mData)->type;

    return sSceneComponents[type].load(sScene, mData, props, err);
}

const char* ComponentView::get_name()
{
    return (*mData)->name;
}

void ComponentView::set_name(const char* cstr)
{
    sScene->active->registry.set_component_name((*mData)->cuid, cstr);
}

AssetID ComponentView::get_asset_id(uint32_t assetSlotIndex)
{
    return scene_component_get_asset(sScene, mData, assetSlotIndex);
}

bool ComponentView::set_asset_id(uint32_t assetSlotIndex, AssetID assetID)
{
    return scene_component_set_asset(sScene, mData, assetSlotIndex, assetID);
}

AssetType ComponentView::get_asset_type(uint32_t assetSlotIndex)
{
    return scene_component_get_asset_type(sScene, (*mData)->type, assetSlotIndex);
}

AssetID ComponentView::get_script_asset_id()
{
    return (*mData)->scriptAssetID;
}

void ComponentView::set_script_asset_id(AssetID assetID)
{
    // TODO: check AssetType???

    (*mData)->scriptAssetID = assetID;
}

void ComponentView::get_children(Vector<ComponentView>& children)
{
    children.clear();

    // kinda slow with ID -> Data pointer chasing.
    for (ComponentBase* child = (*mData)->child; child; child = child->next)
    {
        ComponentView childC(sScene->active->registry.get_component_data(child->cuid, nullptr));
        LD_ASSERT(childC);
        children.push_back(childC);
    }
}

ComponentView ComponentView::get_parent()
{
    ComponentBase* parentBase = (*mData)->parent;
    if (!parentBase)
        return {};

    return ComponentView(sScene->active->registry.get_component_data(parentBase->cuid, nullptr));
}

bool ComponentView::get_transform(TransformEx& transform)
{
    // TODO: DataRegistry API to take ComponentBase* directly
    return sScene->active->registry.get_component_transform((*mData)->cuid, transform);
}

bool ComponentView::set_transform(const TransformEx& transform)
{
    // TODO: DataRegistry API to take ComponentBase* directly
    return sScene->active->registry.set_component_transform((*mData)->cuid, transform);
}

bool ComponentView::get_transform_2d(Transform2D& transform)
{
    // TODO: DataRegistry API to take ComponentBase* directly
    return sScene->active->registry.get_component_transform_2d((*mData)->cuid, transform);
}

bool ComponentView::set_transform_2d(const Transform2D& transform)
{
    // TODO: DataRegistry API to take ComponentBase* directly
    return sScene->active->registry.set_component_transform_2d((*mData)->cuid, transform);
}

bool ComponentView::get_world_transform_2d(Transform2D& transform)
{
    // TODO: DataRegistry API to take ComponentBase* directly
    return sScene->active->registry.get_component_world_transform_2d((*mData)->cuid, transform);
}

bool ComponentView::get_world_mat4(Mat4& worldMat4)
{
    ComponentBase* base = *mData;

    return sScene->active->registry.get_component_world_mat4(base->cuid, worldMat4);
}

ComponentView Scene::get_2d_component_by_position(const Vec2& worldPos)
{
    CUID compCUID = sScene->renderSystemCache.get_2d_component_by_position(worldPos, [](RUID ruid, Mat4& mat4, void*) -> bool { return Scene(sScene).get_ruid_world_mat4(ruid, mat4); }, nullptr);

    return ComponentView(sScene->active->registry.get_component_data(compCUID, nullptr));
}

ComponentView Scene::get_ruid_component(RUID ruid)
{
    CUID compCUID = mObj->renderSystemCache.get_draw_id_component(ruid);

    return ComponentView(sScene->active->registry.get_component_data(compCUID, nullptr));
}

bool Scene::get_ruid_world_mat4(RUID ruid, Mat4& worldMat4)
{
    ComponentView comp = get_ruid_component(ruid);
    if (!comp || !mObj->active->registry.get_component_world_mat4(comp.cuid(), worldMat4))
        return false;

    return true;
}

bool Scene::get_component_path(ComponentView comp, Vector<int>& path)
{
    if (!comp)
        return false;

    return mObj->active->registry.get_component_path(comp.cuid(), path);
}

void Scene::invalidate(Vec2 extent)
{
    LD_PROFILE_SCOPE;

    mObj->active->invalidate(extent);
}

std::string Scene::print_hierarchy()
{
    return mObj->active->registry.print_hierarchy();
}

} // namespace LD
