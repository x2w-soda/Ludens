set(MODULE_INCLUDE
	"Include/RenderService.h"
)

set(MODULE_LIB
	"Lib/RenderService.cpp"
	"Lib/RenderResources.h"
	"Lib/RenderPassResources.h"
	"Lib/RenderPassResources.cpp"
	"Lib/FrameBufferResources.h"
	"Lib/FrameBufferResources.cpp"
	"Lib/PipelineResources.h"
	"Lib/PipelineResources.cpp"
	"Lib/BindingGroupResources.h"
	"Lib/BindingGroupResources.cpp"
	"Lib/TextureResources.h"
	"Lib/TextureResources.cpp"
	"Lib/RenderContext.h"
	"Lib/RenderContext.cpp"
	"Lib/RenderUI.h"
	"Lib/RenderUI.cpp"
)

set(MODULE_INCLUDE_DIR
	"${CMAKE_SOURCE_DIR}/Ludens"
)

add_library(LDRenderService STATIC
	"${MODULE_INCLUDE}"
	"${MODULE_LIB}"
)

target_include_directories(LDRenderService PRIVATE
	"${MODULE_INCLUDE_DIR}"
)
