#pragma once

#include <Ludens/Header/Handle.h>
#include <LudensEditor/EditorContext/EditStack.h>

namespace LD {

enum EditorActionType
{
    EDITOR_ACTION_UNDO = 0,
    EDITOR_ACTION_REDO,
    EDITOR_ACTION_NEW_SCENE,
    EDITOR_ACTION_OPEN_SCENE,
    EDITOR_ACTION_SAVE_SCENE,
    EDITOR_ACTION_ADD_COMPONENT_SCRIPT,
    EDITOR_ACTION_ENUM_COUNT,
};

struct EditorActionInfo
{
    EditorActionType type;
    void (*action)(EditStack stack, void* user);
    const char* name;
};

/// @brief An editor action maps to one or more editor commands.
struct EditorAction
{
    static void register_action(const EditorActionInfo& info);
};

/// @brief A queue of editor actions to execute sequentially.
struct EditorActionQueue : Handle<struct EditorActionQueueObj>
{
    /// @brief Create the action queue.
    static EditorActionQueue create(EditStack stack, void* user);

    /// @brief Destroy the action queue.
    static void destroy(EditorActionQueue queue);

    /// @brief Add an action to queue for later execution.
    void enqueue(EditorActionType type);

    /// @brief Execute all actions sequentially until queue is empty.
    void poll_actions();
};

} // namespace LD