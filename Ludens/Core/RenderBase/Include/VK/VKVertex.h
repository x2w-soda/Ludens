#pragma once

#include <vulkan/vulkan_core.h>
#include "Core/DSA/Include/Vector.h"

namespace LD
{

using VKVertexBindings = Vector<VkVertexInputBindingDescription>;
using VKVertexAttributes = Vector<VkVertexInputAttributeDescription>;

class VKVertexLayout
{
public:
    VKVertexLayout& AddBinding(const VkVertexInputBindingDescription& binding);
    VKVertexLayout& AddAttribute(const VkVertexInputAttributeDescription& attribute);

    VKVertexBindings& GetBindings()
    {
        return mBindings;
    }
    
    VKVertexAttributes& GetAttributes()
    {
        return mAttributes;
    }
    
    const VKVertexBindings& GetBindings() const
    {
        return mBindings;
    }

    const VKVertexAttributes& GetAttributes() const
    {
        return mAttributes;
    }

private:
    VKVertexBindings mBindings;
    VKVertexAttributes mAttributes;
};

} // namespace LD