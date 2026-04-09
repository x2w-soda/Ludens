#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Memory/Memory.h>
#include <LudensEditor/EditorContext/EditCommand.h>

namespace LD {

struct EditorContextObj;

struct EditStack : Handle<struct EditStackObj>
{
    /// @brief Create the edit command stack.
    static EditStack create(EditorContextObj* ctx);

    /// @brief Destroy the edit command stack.
    static void destroy(EditStack stack);

    /// @brief Clear and free all commands.
    void clear();

    EditCommand* allocate(EditCommandType type);

    /// @brief Execute command and push it onto stack.
    /// @note The command will be freed with when it goes out of scope,
    ///       caller should have created the derived-type EditCommand with new_command.
    void execute(EditCommand* cmd);

    /// @brief Undo a command.
    void undo();

    /// @brief Redo a command.
    void redo();

    /// @brief Get number of commands in stack.
    size_t size() const;

    /// @brief Get command index in stack.
    int index();
};

} // namespace LD
