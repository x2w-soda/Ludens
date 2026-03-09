#pragma once

#include <Ludens/DataRegistry/DataRegistry.h>
#include <Ludens/Header/Handle.h>
#include <Ludens/Serial/SUID.h>
#include <Ludens/System/FileSystem.h>
#include <LudensEditor/EditorContext/EditStack.h>

namespace LD {

enum EditorActionType
{
    EDITOR_ACTION_UNDO = 0,
    EDITOR_ACTION_REDO,
    EDITOR_ACTION_NEW_SCENE,
    EDITOR_ACTION_OPEN_SCENE,
    EDITOR_ACTION_SAVE_SCENE,
    EDITOR_ACTION_OPEN_PROJECT,
    EDITOR_ACTION_ADD_COMPONENT,
    EDITOR_ACTION_ADD_COMPONENT_SCRIPT,
    EDITOR_ACTION_SET_COMPONENT_ASSET,
    EDITOR_ACTION_CLONE_COMPONENT_SUBTREE,
    EDITOR_ACTION_DELETE_COMPONENT_SUBTREE,
    EDITOR_ACTION_CLOSE_DIALOG,
    EDITOR_ACTION_ENUM_COUNT,
};

struct EditorActionAddComponent
{
    SUID parentSUID;
    ComponentType compType;
};

struct EditorActionAddComponentScript
{
    SUID compSUID;
    AssetID assetID;
};

using EditorActionSetComponentAsset = EditorActionAddComponentScript;

/// @brief An editor action maps to zero or more editor commands.
struct EditorAction
{
    EditorActionType type;
    union
    {
        FS::Path newScene, openScene, openProject;
        EditorActionAddComponent addComponent;
        EditorActionAddComponentScript addComponentScript;
        EditorActionSetComponentAsset setComponentAsset;
        SUID cloneComponentSubtree;
        SUID deleteComponentSubtree;
    };

    EditorAction() = delete;
    EditorAction(EditorActionType type);
    EditorAction(const EditorAction&) = delete;
    EditorAction(EditorAction&&) = delete;
    ~EditorAction();

    EditorAction& operator=(const EditorAction&) = delete;
    EditorAction& operator=(EditorAction&&) = delete;
};

struct EditorActionInfo
{
    EditorActionType type;
    void (*action)(EditStack stack, const EditorAction& action, void* user);
    const char* name;
};

void register_editor_action(const EditorActionInfo& info);

/// @brief A queue of editor actions to execute sequentially.
struct EditorActionQueue : Handle<struct EditorActionQueueObj>
{
    /// @brief Create the action queue.
    static EditorActionQueue create(EditStack stack, void* user);

    /// @brief Destroy the action queue and all allocated EditorActions.
    static void destroy(EditorActionQueue queue);

    /// @brief Allocate an action in queue for later execution.
    /// @return An editor action with requested type, this is owned by queue.
    EditorAction* enqueue(EditorActionType type);

    /// @brief Execute all actions sequentially until queue is empty.
    void poll_actions();
};

} // namespace LD
