#include <Ludens/System/Memory.h>
#include <LudensEditor/EditorContext/EditStack.h>
#include <vector>

namespace LD {

/// @brief Edit stack implementation. Maintains a stack of
///        EditCommands for undo and redo.
struct EditStackObj
{
    std::vector<EditCommand*> commands;
    size_t index = 0;
};

EditStack EditStack::create()
{
    auto* obj = heap_new<EditStackObj>(MEMORY_USAGE_MISC);

    return EditStack(obj);
}

void EditStack::destroy(EditStack stack)
{
    auto* obj = (EditStackObj*)stack.unwrap();

    stack.clear();

    heap_delete<EditStackObj>(obj);
}

void EditStack::clear()
{
    for (EditCommand* cmd : mObj->commands)
    {
        // this depends on how EditStack::new_command allocates
        heap_delete<EditCommand>(cmd);
    }

    mObj->commands.clear();
}

bool EditStack::execute(EditCommand* cmd)
{
    if (!cmd->mIsValid)
        return false;

    cmd->redo();

    if (mObj->index < mObj->commands.size())
        mObj->commands.erase(mObj->commands.begin() + mObj->index, mObj->commands.end());

    mObj->commands.push_back(cmd);
    mObj->index++;

    return true;
}

void EditStack::undo()
{
    if (mObj->index == 0)
        return;

    mObj->commands[mObj->index - 1]->undo();
    mObj->index--;
}

void EditStack::redo()
{
    if (mObj->index >= mObj->commands.size())
        return;

    mObj->commands[mObj->index]->redo();
    mObj->index++;
}

size_t EditStack::size() const
{
    return mObj->commands.size();
}

} // namespace LD