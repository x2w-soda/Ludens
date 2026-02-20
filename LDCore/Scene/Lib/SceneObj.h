#pragma once

#include <Ludens/UI/UIContext.h>

#include "AudioSystemCache.h"
#include "LuaScript.h"
#include "RenderSystemCache.h"
#include "ScreenUI.h"

// Scene user's responsibility to check handle before calling methods
#define LD_ASSERT_COMPONENT_LOADED(DATA) LD_ASSERT(DATA && *(DATA) && ((*(DATA))->flags & COMPONENT_FLAG_LOADED_BIT))

namespace LD {

enum SceneState
{
    SCENE_STATE_EMPTY = 0,
    SCENE_STATE_LOADED,
    SCENE_STATE_RUNNING,
};

/// @brief Scene implementation.
class SceneObj
{
public:
    DataRegistry registry;
    DataRegistry registryBackup;
    AssetManager assetManager{};
    AudioSystemCache audioSystemCache;
    RenderSystemCache renderSystemCache;
    LuaScript::Context luaContext{};
    LD::ScreenUI screenUI;
    LD::ScreenUI screenUIBackup;
    CameraComponent* mainCameraC;
    Vec2 extent = {};
    SceneState state = SCENE_STATE_EMPTY;

    void load_registry_from_backup();
    void unload_registry();
    void startup_registry();
    void cleanup_registry();
    void resize(const Vec2& extent);

private:
    bool load_subtree_from_backup(ComponentBase** dstData, ComponentBase** srcData);

    /// @brief Unload components recursively, destroying resources from systems/servers.
    void unload_subtree(ComponentBase** data);

    /// @brief Startup a component subtree recursively, attaching scripts to components
    void startup_subtree(ComponentBase** data);

    /// @brief Cleanup a component subtree recursively, detaching scripts from components
    void cleanup_subtree(ComponentBase** data);
};

extern SceneObj* sScene;

} // namespace LD