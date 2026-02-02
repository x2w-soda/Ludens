#pragma once

#include "AudioServerCache.h"
#include "LuaScript.h"
#include "RenderServerCache.h"

namespace LD {

enum SceneState
{
    SCENE_STATE_EMPTY = 0,
    SCENE_STATE_LOADED,
    SCENE_STATE_RUNNING,
};

/// @brief Scene implementation.
struct SceneObj
{
    DataRegistry registry;
    DataRegistry registryBack;
    AssetManager assetManager{};
    AudioServerCache audioServerCache;
    RenderServerCache renderServerCache;
    LuaScript::Context luaContext{};
    CameraComponent* mainCameraC;
    CUID mainCameraCUID;
    Vec2 screenExtent = {};
    SceneState state = SCENE_STATE_EMPTY;

    /// @brief Load components recursively, creating resources from systems/servers.
    void load(ComponentBase* comp);

    /// @brief Unload components recursively, destroying resources from systems/servers.
    void unload(ComponentBase* comp);

    /// @brief Startup a component subtree recursively, attaching scripts to components
    void startup_root(CUID compID);

    /// @brief Cleanup a component subtree recursively, detaching scripts from components
    void cleanup_root(CUID compID);
};

extern SceneObj* sScene;

} // namespace LD