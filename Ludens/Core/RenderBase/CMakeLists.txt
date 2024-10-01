set(MODULE_INCLUDE
	"Include/GL/GL.h"
	"Include/GL/GLSL.h"
	"Include/GL/GLContext.h"
	"Include/GL/GLCommand.h"
	"Include/GL/GLVertexArray.h"
	"Include/GL/GLFrameBuffer.h"
	"Include/GL/GLVertex.h"
	"Include/GL/GLTexture.h"
	"Include/GL/GLBuffer.h"
	"Include/GL/GLProgram.h"

	"Include/VK/VK.h"
	"Include/VK/VKSwapChain.h"
	"Include/VK/VKVertex.h"
	"Include/VK/VKBuffer.h"
	"Include/VK/VKImage.h"
	"Include/VK/VKShader.h"
	"Include/VK/VKMemory.h"
	"Include/VK/VKCommand.h"
	"Include/VK/VKContext.h"
	"Include/VK/VKRenderPass.h"
	"Include/VK/VKDescriptor.h"
	"Include/VK/VKDevice.h"
	"Include/VK/VKFence.h"
	"Include/VK/VKFrameBuffer.h"
	"Include/VK/VKFormat.h"
	"Include/VK/VKPipeline.h"
	"Include/VK/VKSemaphore.h"

	"Include/RTypes.h"
	"Include/RResult.h"
	"Include/RDevice.h"
	"Include/RTexture.h"
	"Include/RBuffer.h"
	"Include/RShader.h"
	"Include/RBinding.h"
	"Include/RPass.h"
	"Include/RFrameBuffer.h"
	"Include/RPipeline.h"
)

set(MODULE_LIB
	"Lib/GL/GLSL.cpp"
	"Lib/GL/GLContext.cpp"
	"Lib/GL/GLCommand.cpp"
	"Lib/GL/GLVertexArray.cpp"
	"Lib/GL/GLVertex.cpp"
	"Lib/GL/GLTexture.cpp"
	"Lib/GL/GLBuffer.cpp"
	"Lib/GL/GLProgram.cpp"
	"Lib/GL/GLFrameBuffer.cpp"
	
	"Lib/VK/VKSwapChain.cpp"
	"Lib/VK/VKVertex.cpp"
	"Lib/VK/VKBuffer.cpp"
	"Lib/VK/VKImage.cpp"
	"Lib/VK/VKShader.cpp"
	"Lib/VK/VKMemory.cpp"
	"Lib/VK/VKCommand.cpp"
	"Lib/VK/VKContext.cpp"
	"Lib/VK/VKRenderPass.cpp"
	"Lib/VK/VKDescriptor.cpp"
	"Lib/VK/VKDevice.cpp"
	"Lib/VK/VKFence.cpp"
	"Lib/VK/VKFrameBuffer.cpp"
	"Lib/VK/VKPipeline.cpp"
	"Lib/VK/VKSemaphore.cpp"

	"Lib/RResult.cpp"
	"Lib/RBase.h"
	"Lib/RBase.cpp"
	"Lib/RDevice.cpp"
	"Lib/RDeviceGL.h"
	"Lib/RDeviceGL.cpp"
	"Lib/RDeviceVK.h"
	"Lib/RDeviceVK.cpp"
	"Lib/RTexture.cpp"
	"Lib/RTextureGL.h"
	"Lib/RTextureGL.cpp"
	"Lib/RTextureVK.h"
	"Lib/RTextureVK.cpp"
	"Lib/RBuffer.cpp"
	"Lib/RBufferGL.h"
	"Lib/RBufferGL.cpp"
	"Lib/RBufferVK.h"
	"Lib/RBufferVK.cpp"
	"Lib/RShaderGL.h"
	"Lib/RShaderGL.cpp"
	"Lib/RShaderVK.h"
	"Lib/RShaderVK.cpp"
	"Lib/RBinding.cpp"
	"Lib/RBindingGL.h"
	"Lib/RBindingGL.cpp"
	"Lib/RBindingVK.h"
	"Lib/RBindingVK.cpp"
	"Lib/RPass.cpp"
	"Lib/RPassGL.h"
	"Lib/RPassGL.cpp"
	"Lib/RPassVK.h"
	"Lib/RPassVK.cpp"
	"Lib/RFrameBuffer.cpp"
	"Lib/RFrameBufferGL.h"
	"Lib/RFrameBufferGL.cpp"
	"Lib/RFrameBufferVK.h"
	"Lib/RFrameBufferVK.cpp"
	"Lib/RPipeline.cpp"
	"Lib/RPipelineGL.h"
	"Lib/RPipelineGL.cpp"
	"Lib/RPipelineVK.h"
	"Lib/RPipelineVK.cpp"
	"Lib/RDeriveGL.h"
	"Lib/RDeriveGL.cpp"
	"Lib/RDeriveVK.h"
	"Lib/RDeriveVK.cpp"
)

set(MODULE_INCLUDE_DIR
	"${CMAKE_SOURCE_DIR}/Ludens"
	"${CMAKE_SOURCE_DIR}/Extra/glad/include"
	"${CMAKE_SOURCE_DIR}/Extra/glfw/include"
	"${Vulkan_INCLUDE_DIR}"
)

add_library(LDRenderBase STATIC
	"${MODULE_INCLUDE}"
	"${MODULE_LIB}"
)

target_include_directories(LDRenderBase PRIVATE
	"${MODULE_INCLUDE_DIR}"
)

target_link_libraries(LDRenderBase PRIVATE
	glad
	glfw
)

if(MSVC)
	target_link_libraries(LDRenderBase PRIVATE
		"${Vulkan_LIBRARY_DIR}/vulkan-1.lib"
		optimized "${Vulkan_LIBRARY_DIR}/glslang.lib"
		optimized "${Vulkan_LIBRARY_DIR}/glslang-default-resource-limits.lib"
		optimized "${Vulkan_LIBRARY_DIR}/MachineIndependent.lib"
		optimized "${Vulkan_LIBRARY_DIR}/GenericCodeGen.lib"
		optimized "${Vulkan_LIBRARY_DIR}/OSDependent.lib"
		optimized "${Vulkan_LIBRARY_DIR}/SPIRV.lib"
		optimized "${Vulkan_LIBRARY_DIR}/SPIRV-Tools.lib"
		optimized "${Vulkan_LIBRARY_DIR}/SPIRV-Tools-link.lib"
		optimized "${Vulkan_LIBRARY_DIR}/SPIRV-Tools-opt.lib"
		optimized "${Vulkan_LIBRARY_DIR}/spirv-cross-core.lib"
		optimized "${Vulkan_LIBRARY_DIR}/spirv-cross-glsl.lib"
		optimized "${Vulkan_LIBRARY_DIR}/spirv-cross-reflect.lib"
		debug     "${Vulkan_LIBRARY_DIR}/glslangd.lib"
		debug     "${Vulkan_LIBRARY_DIR}/glslang-default-resource-limitsd.lib"
		debug     "${Vulkan_LIBRARY_DIR}/MachineIndependentd.lib"
		debug     "${Vulkan_LIBRARY_DIR}/GenericCodeGend.lib"
		debug     "${Vulkan_LIBRARY_DIR}/OSDependentd.lib"
		debug     "${Vulkan_LIBRARY_DIR}/SPIRVd.lib"
		debug     "${Vulkan_LIBRARY_DIR}/SPIRV-Toolsd.lib"
		debug     "${Vulkan_LIBRARY_DIR}/SPIRV-Tools-linkd.lib"
		debug     "${Vulkan_LIBRARY_DIR}/SPIRV-Tools-optd.lib"
		debug     "${Vulkan_LIBRARY_DIR}/spirv-cross-cored.lib"
		debug     "${Vulkan_LIBRARY_DIR}/spirv-cross-glsld.lib"
		debug     "${Vulkan_LIBRARY_DIR}/spirv-cross-reflectd.lib"
	)
endif()

set(MODULE_TEST
	"Tests/RenderBaseTests.cpp"
)

add_executable(LDRenderBaseTests
	"${MODULE_TEST}"
)

target_include_directories(LDRenderBaseTests PRIVATE
	"${MODULE_INCLUDE_DIR}"
	"${CMAKE_SOURCE_DIR}/Extra/doctest"
)

target_link_libraries(LDRenderBaseTests PRIVATE
	LDRenderBase
	glad
	glfw
)