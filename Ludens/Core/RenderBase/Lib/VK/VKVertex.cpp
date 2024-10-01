#include "Core/RenderBase/Include/VK/VKVertex.h"

namespace LD
{

VKVertexLayout& VKVertexLayout::AddBinding(const VkVertexInputBindingDescription& binding)
{
    mBindings.PushBack(binding);
    return *this;
}

VKVertexLayout& VKVertexLayout::AddAttribute(const VkVertexInputAttributeDescription& attribute)
{
    mAttributes.PushBack(attribute);
    return *this;
}

} // namespace LD