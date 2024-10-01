set(MODULE_INCLUDE
	"Include/Passes/ColorPass.h"
	"Include/Passes/GBufferPass.h"
	"Include/Passes/SSAOPass.h"
	"Include/FrameBuffers/ColorBuffer.h"
	"Include/FrameBuffers/GBuffer.h"
	"Include/FrameBuffers/SSAOBuffer.h"
	"Include/Groups/FrameStaticGroup.h"
	"Include/Groups/MaterialGroup.h"
	"Include/Groups/ViewportGroup.h"
	"Include/Groups/CubemapGroup.h"
	"Include/Groups/RectGroup.h"
	"Include/Groups/SSAOGroup.h"
	"Include/Groups/ToneMappingGroup.h"
	"Include/Pipelines/RectPipeline.h"
	"Include/Pipelines/LinePipeline.h"
	"Include/Pipelines/CubemapPipeline.h"
	"Include/Pipelines/GBufferPipeline.h"
	"Include/Pipelines/DeferredBlinnPhongPipeline.h"
	"Include/Pipelines/DeferredBRDFPipeline.h"
	"Include/Pipelines/DeferredSSAOPipeline.h"
	"Include/Pipelines/SSAOBlurPipeline.h"
	"Include/Pipelines/SwapChainTransferPipeline.h"
	"Include/Pipelines/PostProcess/ToneMappingPipeline.h"
	"Include/Pipelines/PostProcess/FXAAPipeline.h"
	"Include/PrefabRenderPass.h"
	"Include/PrefabBindingGroup.h"
	"Include/PrefabFrameBuffer.h"
	"Include/PrefabPipeline.h"
	"Include/RShaderCompiler.h"
	"Include/RMesh.h"
	"Include/RFont.h"
	"Include/RBatch.h"
)

set(MODULE_LIB
	"Lib/Passes/ColorPass.cpp"
	"Lib/Passes/GBufferPass.cpp"
	"Lib/Passes/SSAOPass.cpp"
	"Lib/FrameBuffers/ColorBuffer.cpp"
	"Lib/FrameBuffers/GBuffer.cpp"
	"Lib/FrameBuffers/SSAOBuffer.cpp"
	"Lib/Groups/FrameStaticGroup.cpp"
	"Lib/Groups/MaterialGroup.cpp"
	"Lib/Groups/ViewportGroup.cpp"
	"Lib/Groups/CubemapGroup.cpp"
	"Lib/Groups/RectGroup.cpp"
	"Lib/Groups/SSAOGroup.cpp"
	"Lib/Groups/ToneMappingGroup.cpp"
	"Lib/Pipelines/RectPipeline.cpp"
	"Lib/Pipelines/LinePipeline.cpp"
	"Lib/Pipelines/CubemapPipeline.cpp"
	"Lib/Pipelines/GBufferPipeline.cpp"
	"Lib/Pipelines/DeferredBlinnPhongPipeline.cpp"
	"Lib/Pipelines/DeferredBRDFPipeline.cpp"
	"Lib/Pipelines/DeferredSSAOPipeline.cpp"
	"Lib/Pipelines/SSAOBlurPipeline.cpp"
	"Lib/Pipelines/SwapChainTransferPipeline.cpp"
	"Lib/Pipelines/PostProcess/ToneMappingPipeline.cpp"
	"Lib/Pipelines/PostProcess/FXAAPipeline.cpp"
	"Lib/RShaderCompiler.cpp"
	"Lib/RMesh.cpp"
	"Lib/RFont.cpp"
)

set(MODULE_INCLUDE_DIR
	"${CMAKE_SOURCE_DIR}/Ludens"
)

add_library(LDRenderFX STATIC
	"${MODULE_INCLUDE}"
	"${MODULE_LIB}"
)

target_link_libraries(LDRenderFX PRIVATE
	stb
)

target_include_directories(LDRenderFX PRIVATE
	"${MODULE_INCLUDE_DIR}"
)
