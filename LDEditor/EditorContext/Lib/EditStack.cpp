#include <Ludens/DSA/Vector.h>
#include <Ludens/Memory/Memory.h>
#include <LudensEditor/EditorContext/EditStack.h>

namespace LD {

/// @brief Edit stack implementation. Maintains a stack of
///        EditCommands for undo and redo.
struct EditStackObj
{
    EditorContextObj* ctx = nullptr;
    Vector<EditCommand*> commands;
    size_t index = 0;
};

EditStack EditStack::create(EditorContextObj* ctx)
{
    auto* obj = heap_new<EditStackObj>(MEMORY_USAGE_MISC);

    obj->ctx = ctx;

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
        EditCommand::destroy(cmd);

    mObj->commands.clear();
}

EditCommand* EditStack::allocate(EditCommandType type)
{
    return EditCommand::create(type, mObj->ctx);
}

void EditStack::execute(EditCommand* cmd)
{
    LD_ASSERT(cmd);

    EditCommand::redo(cmd);

    if (mObj->index < mObj->commands.size())
        mObj->commands.erase(mObj->commands.begin() + mObj->index, mObj->commands.end());

    mObj->commands.push_back(cmd);
    mObj->index++;
}

void EditStack::undo()
{
    if (mObj->index == 0)
        return;

    EditCommand::undo(mObj->commands[mObj->index - 1]);
    mObj->index--;
}

void EditStack::redo()
{
    if (mObj->index >= mObj->commands.size())
        return;

    EditCommand::redo(mObj->commands[mObj->index]);
    mObj->index++;
}

size_t EditStack::size() const
{
    return mObj->commands.size();
}

} // namespace LD