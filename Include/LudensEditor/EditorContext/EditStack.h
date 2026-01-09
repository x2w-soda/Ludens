#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Memory/Memory.h>

namespace LD {

class EditCommand
{
    friend class EditStack;

public:
    virtual ~EditCommand() = default;

    /// @brief Derived class implements command.
    virtual void redo() = 0;

    /// @brief Derived class implements the inverse of the command.
    virtual void undo() = 0;

private:
    bool mIsValid = false;
};

struct EditStack : Handle<struct EditStackObj>
{
    /// @brief Allocate an EditCommand.
    template <typename TEditCommand, typename... TArgs>
    static inline TEditCommand* new_command(TArgs&&... args)
    {
        TEditCommand* derived = heap_new<TEditCommand>(MEMORY_USAGE_MISC, std::forward<TArgs>(args)...);
        derived->mIsValid = true;
        return derived;
    }

    /// @brief Create the edit command stack.
    static EditStack create();

    /// @brief Destroy the edit command stack.
    static void destroy(EditStack stack);

    /// @brief Clear and free all commands.
    void clear();

    /// @brief Execute command and push it onto stack.
    /// @return Whether command is successfully executed, rejection could happen
    ///         if the command wasn't allocated via EditStack::new_command.
    /// @note The command will be freed with when it goes out of scope,
    ///       caller should have created the derived-type EditCommand with new_command.
    bool execute(EditCommand* cmd);

    /// @brief Undo a command.
    void undo();

    /// @brief Redo a command.
    void redo();

    /// @brief Get number of commands in stack.
    size_t size() const;
};

} // namespace LD
