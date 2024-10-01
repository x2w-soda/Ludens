#include <iostream>
#include <cstdlib>
#include <string>
#include "Core/Header/Include/Error.h"
#include "Core/RenderBase/Include/VK/VKInfo.h"
#include "Core/RenderBase/Include/VK/VKContext.h"
#include "Core/RenderBase/Include/VK/VKShader.h"
#include "Core/RenderBase/Include/VK/VKDevice.h"

namespace LD
{

VKShader::~VKShader()
{
    LD_DEBUG_ASSERT(mHandle == VK_NULL_HANDLE);
    LD_DEBUG_ASSERT(mDevice == VK_NULL_HANDLE);
}

VKShader& VKShader::SetName(const char* shaderName)
{
    if (shaderName)
    {
        mShaderName = shaderName;
    }

    return *this;
}

VKShader& VKShader::SetShaderSource(u32 size, const u32* source)
{
    mShaderSize = size;
    mShaderSource = source;

    return *this;
}

VKShader& VKShader::SetShaderStage(VkShaderStageFlagBits stage)
{
    mShaderStage = stage;

    return *this;
}

void VKShader::Startup(const VKContext& context)
{
    LD_DEBUG_ASSERT(mShaderSource && mShaderSize > 0);

    mDevice = context.GetDevice().GetHandle();

    VkShaderModuleCreateInfo moduleCI{};
    moduleCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCI.codeSize = mShaderSize;
    moduleCI.pCode = mShaderSource;

    if (vkCreateShaderModule(mDevice, &moduleCI, nullptr, &mHandle) != VK_SUCCESS)
    {
        std::cout << "vkCreateShaderModule failed" << std::endl;
        return;
    }

    // release handles to the SPIR-V byte code
    mShaderSource = nullptr;
    mShaderSize = 0;

    std::cout << "VKShader " << mShaderName.c_str() << " setup complete" << std::endl;

    mHasStartup = true;
}

void VKShader::Cleanup()
{
    vkDestroyShaderModule(mDevice, mHandle, nullptr);
    mDevice = VK_NULL_HANDLE;
    mHandle = VK_NULL_HANDLE;

    std::cout << "VKShader " << mShaderName.c_str() << " cleanup complete" << std::endl;

    mHasStartup = false;
}

} // namespace LD