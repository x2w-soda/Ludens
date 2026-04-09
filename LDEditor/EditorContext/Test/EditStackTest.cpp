#include <Extra/doctest/doctest.h>
#include <Ludens/Memory/Memory.h>
#include <LudensEditor/EditorContext/EditStack.h>

#include "EditorContextTest.h"

using namespace LD;

TEST_CASE("EditStack")
{
    // TODO: EditStack is now coupled with EditorContext,
    //       we need at least a headless context to test the EditStack,
    //       but at that point maybe just test the EditorEventQueue
    /*
    EditStack stk = EditStack::create();
    EditStack::destroy(stk);
    */
}
