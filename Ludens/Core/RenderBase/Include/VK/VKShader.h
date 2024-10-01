#pragma once

#include <string>
#include <vulkan/vulkan_core.h>
#include "Core/Header/Include/Types.h"
#include "Core/DSA/Include/Vector.h"

namespace LD
{

class VKContext;

class VKShader
{
public:
    VKShader() = default;
    VKShader(const VKShader&) = delete;
    ~VKShader();

    VKShader& operator=(const VKShader&) = delete;

    VKShader& SetName(const char* shaderName);
    VKShader& SetShaderSource(u32 size, const u32* source);
    VKShader& SetShaderStage(VkShaderStageFlagBits stage);

    inline VkShaderModule GetHandle() const
    {
        return mHandle;
    }

    inline VkShaderStageFlagBits GetStage() const
    {
        return mShaderStage;
    }

    void Startup(const VKContext& context);
    void Cleanup();

private:
    bool mHasStartup = false;
    std::string mShaderName;
    u32 mShaderSize;
    const u32* mShaderSource;
    VkShaderStageFlagBits mShaderStage;

    VkDevice mDevice = VK_NULL_HANDLE;
    VkShaderModule mHandle = VK_NULL_HANDLE;
};

} // namespace LD