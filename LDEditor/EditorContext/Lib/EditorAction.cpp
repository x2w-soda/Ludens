#include <LudensEditor/EditorContext/EditorAction.h>
#include <queue>

namespace LD {

/// @brief Editor action queue implementation.
struct EditorActionQueueObj
{
    std::queue<EditorActionType> actionQueue;
    EditStack editStack;
    void* user;
};

static EditorActionInfo sActions[EDITOR_ACTION_ENUM_COUNT];

void EditorAction::register_action(const EditorActionInfo& info)
{
    sActions[(int)info.type] = info;
}

EditorActionQueue EditorActionQueue::create(EditStack stack, void* user)
{
    auto* obj = heap_new<EditorActionQueueObj>(MEMORY_USAGE_MISC);
    obj->editStack = stack;
    obj->user = user;

    return EditorActionQueue(obj);
}

void EditorActionQueue::destroy(EditorActionQueue queue)
{
    auto* obj = queue.unwrap();

    heap_delete<EditorActionQueueObj>(obj);
}

void EditorActionQueue::enqueue(EditorActionType type)
{
    mObj->actionQueue.push(type);
}

void EditorActionQueue::poll_actions()
{
    while (!mObj->actionQueue.empty())
    {
        EditorActionType type = mObj->actionQueue.front();
        mObj->actionQueue.pop();

        if (sActions[(int)type].action)
        {
            sActions[(int)type].action(mObj->editStack, mObj->user);
        }
    }
}

} // namespace LD
