#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/UI/UIContext.h>

#include "AudioSystemCache.h"
#include "LuaScript.h"
#include "RenderSystemCache.h"
#include "ScreenUI.h"

namespace LD {

enum SceneState
{
    SCENE_STATE_EMPTY = 0,
    SCENE_STATE_LOADED,
    SCENE_STATE_RUNNING,
};

enum SceneContextType
{
    SCENE_CONTEXT_ACTIVE,
    SCENE_CONTEXT_SHADOW,
    SCENE_CONTEXT_BACKUP,
};

struct SceneContextInfo
{
    UIFont uiFont;
    UITheme uiTheme;
};

struct SceneContext
{
    LuaScript::Context lua;
    ScreenUI screenUI;
    DataRegistry registry;
    Vector<CUID> destructionQueue;

    SceneContext() = delete;
    SceneContext(const SceneContextInfo& info);
    SceneContext(const SceneContext&) = delete;
    ~SceneContext();

    SceneContext& operator=(const SceneContext&) = delete;

    // Intended for editor to invalidate without simulating the scene.
    void invalidate(Vec2 extent);
    void invalidate_cameras(Vec2 extent);

    void update(const SceneUpdateTick& tick);
    void post_update();

    bool startup_registry();
    void cleanup_registry();
    void unload_registry(SUIDRegistry suidRegistry);

    /// @brief Startup a component subtree recursively, attaching scripts to components
    bool startup_subtree(ComponentBase** data, Vector<ComponentBase**>& startupOrder, std::string& err);

    bool startup_component(ComponentBase** data, std::string& err);

    /// @brief Cleanup a component subtree recursively, detaching scripts from components
    void cleanup_subtree(ComponentBase** data);

    bool cleanup_component(ComponentBase** data, std::string& err);

    void queue_component_destruction(CUID compID);

    /// @brief Unload components recursively, destroying resources from systems/servers.
    void unload_subtree(ComponentBase** data, SUIDRegistry suidRegistry);
};

/// @brief Scene implementation.
class SceneObj
{
public:
    SceneContext* active;
    SceneContext* shadow;
    SceneContext* backup;
    SceneContextType contextTarget = SCENE_CONTEXT_ACTIVE;
    SceneContextInfo contextInfo = {};
    SUIDRegistry suidRegistry = {};
    AudioSystemCache audioSystemCache;
    RenderSystemCache renderSystemCache;
    SceneState state = SCENE_STATE_EMPTY;
    SceneUpdateTick tick = {};

    struct Transition
    {
        bool inProgress = false;
        SceneLoadFn loadFn;
    } transition;

    bool load_registry_from_backup();
    bool clone_subtree(ComponentBase** dstData, ComponentBase** srcData, std::string& err);

private:
    bool load_subtree_from_backup(ComponentBase** dstData, ComponentBase** srcData, std::string& err);
};

extern SceneObj* sScene;

} // namespace LD