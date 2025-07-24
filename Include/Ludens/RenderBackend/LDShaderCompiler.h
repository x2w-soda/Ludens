#include <Ludens/RenderBackend/LDShaderParser.h>
#include <cstdint>
#include <vector>

namespace LD {

/// @brief A ldshader backend that targets SPIRV for Vulkan.
///        The output SPIRV code may be supplied to VkShaderModuleCreateInfo on success.
struct LDShaderCompilerVulkan : Handle<struct LDShaderCompilerVulkanObj>
{
    static LDShaderCompilerVulkan create();
    static void destroy(LDShaderCompilerVulkan compiler);

    bool compile(LDShaderAST ast, std::vector<uint32_t>& spirv);
};

} // namespace LD