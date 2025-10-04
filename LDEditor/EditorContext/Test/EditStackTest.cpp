#include "EditorContextTest.h"
#include <Extra/doctest/doctest.h>
#include <Ludens/System/Memory.h>
#include <LudensEditor/EditorContext/EditStack.h>

using namespace LD;

static int sCounter = 0;
static int sDtorCounter = 0;

class IncEdit : public EditCommand
{
public:
    IncEdit() = delete;
    IncEdit(int value)
        : mValue(value) {}
    ~IncEdit()
    {
        sDtorCounter++;
    }

    virtual void redo() override
    {
        sCounter += mValue;
    }

    virtual void undo() override
    {
        sCounter -= mValue;
    }

private:
    int mValue;
};

TEST_CASE("EditStack basic")
{
    EditStack stk = EditStack::create();

    sDtorCounter = 0;
    sCounter = 0;

    stk.execute(EditStack::new_command<IncEdit>(30));
    CHECK(stk.size() == 1);

    CHECK(sCounter == 30);

    // nothing to redo
    stk.redo();
    CHECK(sCounter == 30);

    stk.undo();
    CHECK(sCounter == 0);

    stk.redo();
    CHECK(sCounter == 30);

    stk.execute(EditStack::new_command<IncEdit>(40));
    CHECK(sCounter == 70);
    CHECK(stk.size() == 2);

    stk.execute(EditStack::new_command<IncEdit>(30));
    CHECK(sCounter == 100);
    CHECK(stk.size() == 3);

    stk.undo();
    CHECK(sCounter == 70);

    stk.undo();
    CHECK(sCounter == 30);

    stk.undo();
    CHECK(sCounter == 0);

    stk.redo();
    stk.redo();
    stk.redo();
    CHECK(sCounter == 100);

    EditStack::destroy(stk);

    CHECK(sDtorCounter == 3);

    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}

TEST_CASE("EditStack invalid")
{
    EditStack stk = EditStack::create();

    sDtorCounter = 0;
    sCounter = 0;

    IncEdit* invalid = new IncEdit(30);
    bool result = stk.execute(invalid);
    CHECK_FALSE(result);
    CHECK(stk.size() == 0);

    delete invalid;

    EditStack::destroy(stk);
}