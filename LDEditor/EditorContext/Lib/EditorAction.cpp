#include <Ludens/DSA/Queue.h>
#include <Ludens/Profiler/Profiler.h>
#include <LudensEditor/EditorContext/EditorAction.h>

namespace LD {

static EditorActionInfo sActions[EDITOR_ACTION_ENUM_COUNT];

EditorAction::EditorAction(EditorActionType type)
    : type(type)
{
    switch (type)
    {
    case EDITOR_ACTION_OPEN_SCENE:
    case EDITOR_ACTION_SAVE_SCENE:
    case EDITOR_ACTION_OPEN_PROJECT:
        new (&openScene.schemaPath) FS::Path();
        break;
    }
}

EditorAction::~EditorAction()
{
    switch (type)
    {
    case EDITOR_ACTION_OPEN_SCENE:
    case EDITOR_ACTION_SAVE_SCENE:
    case EDITOR_ACTION_OPEN_PROJECT:
        openScene.schemaPath.~path();
        break;
    }
}

/// @brief Editor action queue implementation.
struct EditorActionQueueObj
{
    Queue<EditorAction*> actionQueue;
    PoolAllocator actionPA;
    EditStack editStack;
    void* user;
};

void register_editor_action(const EditorActionInfo& info)
{
    sActions[(int)info.type] = info;
}

EditorActionQueue EditorActionQueue::create(EditStack stack, void* user)
{
    auto* obj = heap_new<EditorActionQueueObj>(MEMORY_USAGE_MISC);
    obj->editStack = stack;
    obj->user = user;

    PoolAllocatorInfo paI{};
    paI.blockSize = sizeof(EditorAction);
    paI.isMultiPage = true;
    paI.pageSize = 128;
    paI.usage = MEMORY_USAGE_MISC;
    obj->actionPA = PoolAllocator::create(paI);

    return EditorActionQueue(obj);
}

void EditorActionQueue::destroy(EditorActionQueue queue)
{
    auto* obj = queue.unwrap();

    for (auto it = obj->actionPA.begin(); it; ++it)
    {
        EditorAction* action = (EditorAction*)it.data();
        action->~EditorAction();
    }
    PoolAllocator::destroy(obj->actionPA);

    heap_delete<EditorActionQueueObj>(obj);
}


EditorAction* EditorActionQueue::enqueue(EditorActionType type)
{
    auto* action = (EditorAction*)mObj->actionPA.allocate();
    new (action)EditorAction(type);

    mObj->actionQueue.push(action);

    return action;
}

void EditorActionQueue::poll_actions()
{
    LD_PROFILE_SCOPE;

    while (!mObj->actionQueue.empty())
    {
        EditorAction* action = mObj->actionQueue.front();
        mObj->actionQueue.pop();

        if (sActions[(int)action->type].action)
        {
            sActions[(int)action->type].action(mObj->editStack, *action, mObj->user);
        }

        action->~EditorAction();
        mObj->actionPA.free(action);
    }
}

} // namespace LD
