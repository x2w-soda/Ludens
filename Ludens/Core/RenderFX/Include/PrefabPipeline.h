#pragma once

#include "Core/RenderBase/Include/RPipeline.h"

namespace LD
{

/// base class for first-party graphics pipelines
class PrefabPipeline
{
public:
    PrefabPipeline() = default;
    PrefabPipeline(const PrefabPipeline&) = delete;
    virtual ~PrefabPipeline()
    {
        LD_DEBUG_ASSERT(!mHandle);
    }

    PrefabPipeline& operator=(const PrefabPipeline&) = delete;

    operator bool() const
    {
        return (bool)mHandle;
    }

    explicit operator RPipeline() const
    {
        LD_DEBUG_ASSERT(mHandle);
        return mHandle;
    }

    virtual RPipelineLayoutData GetLayoutData() const = 0;

    int GetUniformBufferCount() const
    {
        return GetBindingCount(RBindingType::UniformBuffer);
    }

    int GetTextureCount() const
    {
        return GetBindingCount(RBindingType::Texture);
    }

protected:
    RPipeline mHandle;

private:
    int GetBindingCount(RBindingType type) const
    {
        RPipelineLayoutData layout = GetLayoutData();
        int counter = 0;

        for (const auto& group : layout.GroupLayouts)
        {
            for (const auto& binding : group)
            {
                if (binding.Type == type)
                    counter += binding.Count;
            }
        }

        return counter;
    }
};

} // namespace LD