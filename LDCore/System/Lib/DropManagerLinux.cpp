#include <Ludens/Header/Platform.h>
#ifdef LD_PLATFORM_LINUX
#include <Ludens/System/DropManager.h>

namespace LD {

DropTarget DropTarget::create(GLFWwindow*, DropTargetFileCallback, void*)
{
    return {};
}

void DropTarget::destroy(DropTarget target)
{
}

} // namespace LD
#endif // LD_PLATFORM_LINUX