#include "EditorContextTest.h"
#include <Extra/doctest/doctest.h>
#include <LudensEditor/EditorContext/EditorAction.h>

using namespace LD;

static int sUndoCounter = 0;

TEST_CASE("EditorActionQueue basic")
{
    sUndoCounter = 0;
    
    EditStack stack = EditStack::create();
    EditorActionQueue queue = EditorActionQueue::create(stack, nullptr);

    EditorActionInfo actionI{};
    actionI.name = "Undo";
    actionI.type = EDITOR_ACTION_UNDO;
    actionI.action = [](EditStack stack, void* user) { sUndoCounter++; };
    EditorAction::register_action(actionI);

    queue.enqueue(EDITOR_ACTION_UNDO);

    CHECK(sUndoCounter == 0);
    queue.poll_actions();
    CHECK(sUndoCounter == 1);

    queue.enqueue(EDITOR_ACTION_UNDO);
    queue.enqueue(EDITOR_ACTION_UNDO);

    CHECK(sUndoCounter == 1);
    queue.poll_actions();
    CHECK(sUndoCounter == 3);

    EditorActionQueue::destroy(queue);
    EditStack::destroy(stack);
}