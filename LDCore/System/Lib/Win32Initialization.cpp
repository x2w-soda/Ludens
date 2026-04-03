#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Platform.h>
#include <Ludens/System/Win32Initialization.h>

#ifdef LD_PLATFORM_WIN32

#include <oleidl.h>
#pragma comment(lib, "Ole32.lib")

namespace LD {
static bool sHasInitializedOLE = false;

void win32_initialize_ole()
{
    if (sHasInitializedOLE)
        return;

    HRESULT result = OleInitialize(nullptr);
    LD_ASSERT(SUCCEEDED(result));
    sHasInitializedOLE = true;
}
} // namespace LD

#else
namespace LD {
void win32_initialize_ole() {}
} // namespace LD
#endif
