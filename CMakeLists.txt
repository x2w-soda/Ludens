cmake_minimum_required(VERSION 3.26)
project(Ludens LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


## Resolve Dependencies
## - Find Vulkan as CMake package
## - Find Doxygen as CMake package
## - Build extra, third-party dependencies


# find Vulkan
find_package(Vulkan REQUIRED glslangValidator SPIRV-Tools)
if(!Vulkan_FOUND)
	message(FATAL_ERROR "Vulkan is not found on the syStem")
endif()

cmake_path(SET Vulkan_LIBRARY_PATH "${Vulkan_LIBRARY}")
cmake_path(GET Vulkan_LIBRARY_PATH PARENT_PATH Vulkan_LIBRARY_DIR)
message(STATUS "LUDENS VULKAN VERSION               ${Vulkan_VERSION}")
message(STATUS "LUDENS VULKAN INCLUDE DIRECTORY     ${Vulkan_INCLUDE_DIR}")
message(STATUS "LUDENS VULKAN LIBRARY DIRECTORY     ${Vulkan_LIBRARY_DIR}")
message(STATUS "LUDENS VULKAN GLSLANG VALIDATOR     ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE}")

# find Doxygen
find_package(Doxygen)
if(!DOXYGEN_FOUND)
	message(FATAL_ERROR "Doxygen is not found on the syStem")
endif()
message(STATUS "LUDENS DOXYGEN ${DOXYGEN_EXECUTABLE}")

# build third-party dependencies
add_subdirectory("Extra/glfw")
add_subdirectory("Extra/glad")
add_subdirectory("Extra/stb")
add_subdirectory("Extra/lua")
add_subdirectory("Extra/md4c")
add_subdirectory("Extra/tinyobj")
add_subdirectory("Extra/tinygltf")
add_subdirectory("Extra/yoga/yoga")
add_subdirectory("Extra/jolt/Build")


## Build the Builder and Reader
## - the Ludens Builder is a command line utility for offline baking
## - the Ludens Reader is the offline user manual and documentation


# build utility builder
add_subdirectory("Ludens/Builder/Main")

# build offline reader
add_subdirectory("Ludens/Reader/Main")


## Embed Data
## - embed first-party SPIRV shaders
## - embed font TTF data


function(embed_shader Stem)
    message(STATUS "LUDENS CMake embed_shader register build commands for Embed${Stem}.cpp")
	add_custom_command(
		OUTPUT Embed${Stem}.cpp
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Embed/GLSL/${Stem}.glsl
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMAND LudensBuilder shaderc --opengl --vulkan ${CMAKE_CURRENT_SOURCE_DIR}/Embed/GLSL/${Stem}.glsl --output ${CMAKE_CURRENT_BINARY_DIR}/
		COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/Scripts/Embed.py Embed${Stem} ${CMAKE_CURRENT_BINARY_DIR}/${Stem}GLVS.spv ${CMAKE_CURRENT_BINARY_DIR}/${Stem}GLFS.spv ${CMAKE_CURRENT_BINARY_DIR}/${Stem}VKVS.spv ${CMAKE_CURRENT_BINARY_DIR}/${Stem}VKFS.spv
		COMMENT "generating ${Stem}.cpp in ${CMAKE_CURRENT_BINARY_DIR}"
	)
endfunction()

embed_shader(Rect)
embed_shader(Line)
embed_shader(GBuffer)
embed_shader(Cubemap)
embed_shader(DeferredBRDF)
embed_shader(DeferredBlinnPhong)
embed_shader(DeferredSSAO)
embed_shader(SSAOBlur)
embed_shader(SwapChainTransfer)
embed_shader(ToneMapping)

add_custom_target(EmbedSPIRV
	DEPENDS EmbedRect.cpp
	DEPENDS EmbedGBuffer.cpp
	DEPENDS EmbedCubemap.cpp
	DEPENDS EmbedDeferredBRDF.cpp
	DEPENDS EmbedDeferredBlinnPhong.cpp
	DEPENDS EmbedDeferredSSAO.cpp
	DEPENDS EmbedSSAOBlur.cpp
	DEPENDS EmbedSwapChainTransfer.cpp
	DEPENDS EmbedToneMapping.cpp
)

add_library(EmbedSPIRVLib STATIC
	EmbedRect.cpp
	EmbedGBuffer.cpp
	EmbedCubemap.cpp
	EmbedDeferredBRDF.cpp
	EmbedDeferredBlinnPhong.cpp
	EmbedDeferredSSAO.cpp
	EmbedSSAOBlur.cpp
	EmbedSwapChainTransfer.cpp
	EmbedToneMapping.cpp
)

add_dependencies(EmbedSPIRVLib EmbedSPIRV)

function(embed_ttf Stem)
	message(STATUS "LUDENS CMake embed_ttf register build commands for Embed${Stem}.cpp")

	foreach(SubPath IN ITEMS ${ARGN})
		list(APPEND FontPathList ${CMAKE_CURRENT_SOURCE_DIR}/Embed/Fonts/${Stem}/${SubPath})
	endforeach()
	add_custom_command(
		OUTPUT Embed${Stem}.cpp
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/Scripts/Embed.py Embed${Stem} ${FontPathList}
		COMMENT "generating Embed${Stem}.cpp in ${CMAKE_CURRENT_BINARY_DIR}"
	)
endfunction()

embed_ttf(DMSans static/DMSans-Regular.ttf static/DMSans-Bold.ttf static/DMSans-Italic.ttf static/DMSans-BoldItalic.ttf)

add_custom_target(EmbedFonts
	DEPENDS EmbedDMSans.cpp
)

add_library(EmbedFontsLib STATIC
	EmbedDMSans.cpp
)

add_dependencies(EmbedFontsLib EmbedFonts)


## Build Core Modules
## - module dependency graph must be acyclic
## - CMake will ensure there are no cyclic dependencies and resolve the build order


set(LudensCoreModules "")

function(add_ludens_core_module Stem)
	message(STATUS "LUDENS CORE MODULE LD${Stem}")
	add_subdirectory("Ludens/Core/${Stem}")
	set(LudensCoreModules "${LudensCoreModules};LD${Stem}" PARENT_SCOPE)
	foreach(dependency IN ITEMS ${ARGN})
		add_dependencies(LD${Stem} LD${dependency})
		target_link_libraries(LD${Stem} PRIVATE LD${dependency})
	endforeach()
endfunction()

# header only modules are not resolved in linkage dependency graph,
# they can be included and used anywhere.
function(add_ludens_core_module_header Stem)
	message(STATUS "LUDENS CORE MODULE LD${Stem}")
	add_subdirectory("Ludens/Core/${Stem}")
endfunction()

add_ludens_core_module_header(Header)
add_ludens_core_module_header(Math)
add_ludens_core_module_header(DSA)
add_ludens_core_module(OS)
add_ludens_core_module(IO)
add_ludens_core_module(Application)
add_ludens_core_module(CommandLine OS)
add_ludens_core_module(Serialize OS)
add_ludens_core_module(Media)
add_ludens_core_module(UI OS Media)
add_ludens_core_module(Document UI Media Serialize)
add_ludens_core_module(PhysicsBase OS)
add_ludens_core_module(PhysicsService PhysicsBase)
add_ludens_core_module(RenderBase OS Application)
add_ludens_core_module(RenderFX RenderBase Media)
add_ludens_core_module(RenderService RenderFX UI)
add_ludens_core_module(Scene RenderService PhysicsService)

# use interface library to link against all core modules at once
add_library(LudensCore INTERFACE)
target_link_libraries(LudensCore INTERFACE
	${LudensCoreModules}
)

add_dependencies(LudensCore EmbedSPIRV)