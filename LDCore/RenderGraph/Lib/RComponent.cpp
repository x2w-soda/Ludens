#include <Ludens/Header/Assert.h>
#include <Ludens/Memory/Memory.h>

#include "RComponent.h"

namespace LD {

RGraphImageObj* RComponentObj::create_image(NodeType type, const char* nameStr, RFormat format, uint32_t width, uint32_t height, const RSamplerInfo* sampler)
{
    Hash32 imageName(nameStr);

    LD_ASSERT(!images.contains(imageName));

    RGraphImageObj* image = images[imageName] = heap_new<RGraphImageObj>(MEMORY_USAGE_RENDER);
    image->type = type;
    image->name = imageName;
    image->compObj = this;
    image->debugName = nameStr;
    image->format = format;
    image->width = width;
    image->height = height;

    if (sampler)
        image->sampler = *sampler;

    return image;
}

} // namespace LD