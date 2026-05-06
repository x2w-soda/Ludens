#pragma once

#include <Ludens/DSA/String.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Scene/ComponentView.h>
#include <Ludens/Serial/Property.h>

namespace LD {

struct Scene;
struct SceneCommand;

typedef void (*SceneCommandFn)(SceneCommand* cmd, void* user);

enum SceneCommandType
{
    SCENE_COMMAND_TYPE_LOAD_SCENE,
    SCENE_COMMAND_TYPE_SET_PROPS,
    SCENE_COMMAND_TYPE_GET_PROPS,
    SCENE_COMMAND_TYPE_CREATE_COMPONENT,
    SCENE_COMMAND_TYPE_ENUM_COUNT
};

enum SceneCommandCategory
{
    SCENE_COMMAND_CATEGORY_WRITE,
    SCENE_COMMAND_CATEGORY_READ,
};

struct SceneCommand
{
    const SceneCommandType type;
    const SceneCommandCategory category;

    SceneCommand(SceneCommandType type, SceneCommandCategory category)
        : type(type), category(category)
    {
    }

    static const char* get_name(SceneCommandType type);
    static SceneCommandType get_type(const char* name);
};

struct SceneCommandLoadScene : SceneCommand
{
    ComponentSubtreeEntry subtree;
    bool loadSuccess = false;

    SceneCommandLoadScene()
        : SceneCommand(SCENE_COMMAND_TYPE_LOAD_SCENE, SCENE_COMMAND_CATEGORY_WRITE)
    {
    }
};

struct SceneCommandSetProps : SceneCommand
{
    String compPath;
    Vector<PropertyNameValue> props;

    SceneCommandSetProps()
        : SceneCommand(SCENE_COMMAND_TYPE_SET_PROPS, SCENE_COMMAND_CATEGORY_WRITE)
    {
    }
};

struct SceneCommandGetProps : SceneCommand
{
    String compPath;
    Vector<PropertyNameValue> props;

    SceneCommandGetProps()
        : SceneCommand(SCENE_COMMAND_TYPE_GET_PROPS, SCENE_COMMAND_CATEGORY_READ)
    {
    }
};

struct SceneCommandCreateComponent : SceneCommand
{
    String parentPath;                                  // path to parent component
    String compName;                                    // created component name
    ComponentType compType = COMPONENT_TYPE_ENUM_COUNT; // created component type
    Vector<PropertyNameValue> props;                    // properties to create with

    struct Result
    {
        ComponentView compView = {};
    } result;

    SceneCommandCreateComponent()
        : SceneCommand(SCENE_COMMAND_TYPE_CREATE_COMPONENT, SCENE_COMMAND_CATEGORY_WRITE)
    {
    }
};

struct SceneCommandQueue : Handle<struct SceneCommandQueueObj>
{
    static SceneCommandQueue create();
    static void destroy(SceneCommandQueue queue);

    SceneCommand* enqueue(SceneCommandType type);

    /// @brief User callback to observe results of each command after execution.
    void set_response_callback(SceneCommandFn responseCB, void* user);

    /// @brief Applies all commands in queue and flushes. Synchronous.
    void poll_commands(Scene& scene);
};

} // namespace LD