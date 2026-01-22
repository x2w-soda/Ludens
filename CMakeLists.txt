cmake_minimum_required(VERSION 3.20)

project(Ludens LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message(STATUS "CMAKE_CXX_COMPILER_ID:     ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "CMAKE_CXX_COMPILER:        ${CMAKE_CXX_COMPILER}")
message(STATUS "CMAKE_C_COMPILER_ID:       ${CMAKE_C_COMPILER_ID}")
message(STATUS "CMAKE_C_COMPILER:          ${CMAKE_C_COMPILER}")

if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE not set, defaulting to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

message(STATUS "CMAKE_BUILD_TYPE:          ${CMAKE_BUILD_TYPE}")

set(CPP_RTTI_ENABLED OFF CACHE INTERNAL "")

option(LD_COMPILE_WITH_PROFILER "compile with profiler" ON)
message(STATUS "LD_COMPILE_WITH_PROFILER:  ${LD_COMPILE_WITH_PROFILER}")

if (LD_COMPILE_WITH_PROFILER)
    set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
    add_compile_definitions(TRACY_ENABLE=1)
else()
    set(TRACY_ENABLE OFF CACHE BOOL "" FORCE)
    add_compile_definitions(TRACY_ENABLE=0)
endif()
message(STATUS "LUDENS TRACY_ENABLE:       ${TRACY_ENABLE}")

option(LD_BUILD_BENCHMARKS "build benchmarks" ON)
message(STATUS "LD_BUILD_BENCHMARKS:       ${LD_BUILD_BENCHMARKS}")

# Build Environment

set(LUDENS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LUDENS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Include)
set(LUDENS_EXTRA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Extra)
set(LUDENS_BUILD_DIR ${CMAKE_BINARY_DIR})

message(STATUS "LUDENS SOURCE DIR          ${LUDENS_SOURCE_DIR}")
message(STATUS "LUDENS INCLUDE DIR         ${LUDENS_INCLUDE_DIR}")
message(STATUS "LUDENS EXTRA DIR           ${LUDENS_EXTRA_DIR}")
message(STATUS "LUDENS BUILD DIR           ${LUDENS_BUILD_DIR}")

# Fetch dependencies via CMake FetchContent
include(CMake/FetchDependencies.cmake)

# Find dependencies via CMake find_package
include(CMake/FindDependencies.cmake)


# build extra libs
add_subdirectory(${LUDENS_EXTRA_DIR}/stb)

# Util Modules
# - not shipped to end user

add_subdirectory(LDUtil/LudensLFS)

# Core Modules
# - strict dependency ordering
# - user facing API in ${LUDENS_INCLUDE_DIR}

set(LD_CORE_MODULE_FOLDER "LDCore")
add_subdirectory(LDCore/Header)
add_subdirectory(LDCore/Profiler)
add_subdirectory(LDCore/Memory)
add_subdirectory(LDCore/DSA)
add_subdirectory(LDCore/System)
add_subdirectory(LDCore/JobSystem)
add_subdirectory(LDCore/CommandLine)
add_subdirectory(LDCore/DSP)
add_subdirectory(LDCore/Text)
add_subdirectory(LDCore/Serial)
add_subdirectory(LDCore/Lexer)
add_subdirectory(LDCore/Log)
add_subdirectory(LDCore/Event)
add_subdirectory(LDCore/Lua)
add_subdirectory(LDCore/Media)
add_subdirectory(LDCore/WindowRegistry)
add_subdirectory(LDCore/Camera)
add_subdirectory(LDCore/Gizmo)
add_subdirectory(LDCore/RenderBackend)
add_subdirectory(LDCore/RenderGraph)
add_subdirectory(LDCore/RenderComponent)
add_subdirectory(LDCore/RenderServer)
add_subdirectory(LDCore/AudioBackend)
add_subdirectory(LDCore/AudioMixer)
add_subdirectory(LDCore/AudioServer)
add_subdirectory(LDCore/UI)
add_subdirectory(LDCore/Asset)
add_subdirectory(LDCore/DataRegistry)
add_subdirectory(LDCore/Scene)
add_subdirectory(LDCore/Project)

add_library(LDCoreLibs INTERFACE)
target_link_libraries(LDCoreLibs INTERFACE
  LDProfiler
  LDMemory
  LDDSA
  LDSystem
  LDJobSystem
  LDCommandLine
  LDDSP
  LDText
  LDSerial
  LDLexer
  LDLog
  LDEvent
  LDLua
  LDMedia
  LDWindowRegistry
  LDCamera
  LDGizmo
  LDRenderBackend
  LDRenderGraph
  LDRenderComponent
  LDRenderServer
  LDAudioBackend
  LDAudioMixer
  LDAudioServer
  LDUI
  LDAsset
  LDDataRegistry
  LDScene
  LDProject
)

# Ludens Runtime
# - standalone game runtime that is shipped
# - links with LDCoreLibs

set(LD_RUNTIME_MODULE_FOLDER "LDRuntime")
add_subdirectory(LDRuntime/RuntimeContext)
add_subdirectory(LDRuntime/RuntimeMain)

# Ludens Builder
# - standalone command line utility
# - headers in ${LUDENS_INCLUDE_DIR}/LudensBuilder
# - links with LDCoreLibs

set(LD_BUILDER_MODULE_FOLDER "LDBuilder")
add_subdirectory(LDBuilder/RenderUtil)
add_subdirectory(LDBuilder/AudioUtil)
add_subdirectory(LDBuilder/AssetUtil)
add_subdirectory(LDBuilder/Win32Util)
add_subdirectory(LDBuilder/MeshUtil)
add_subdirectory(LDBuilder/DocumentCompiler)

add_library(LDBuilderLibs INTERFACE)
target_link_libraries(LDBuilderLibs INTERFACE
  LDRenderUtil
  LDAudioUtil
  LDAssetUtil
  LDWin32Util
  LDMeshUtil
  LDDocumentCompiler
)

add_subdirectory(LDBuilder/BuilderMain)

# Ludens Editor
# - graphical user interface for editing scenes
# - headers in ${LUDENS_INCLUDE_DIR}/LudensEditor
# - links with LDCoreLibs and LDBuilderLibs

set(LD_EDITOR_MODULE_FOLDER "LDEditor")
add_subdirectory(LDEditor/EditorContext)
add_subdirectory(LDEditor/EditorWidget)
add_subdirectory(LDEditor/TabControlWindow)
add_subdirectory(LDEditor/SelectionWindow)
add_subdirectory(LDEditor/VersionWindow)
add_subdirectory(LDEditor/ConsoleWindow)
add_subdirectory(LDEditor/ViewportWindow)
add_subdirectory(LDEditor/OutlinerWindow)
add_subdirectory(LDEditor/InspectorWindow)
add_subdirectory(LDEditor/EditorUI)

add_library(LDEditorLibs INTERFACE)
target_link_libraries(LDEditorLibs INTERFACE
  LDEditorContext
  LDEditorWidget
  LDTabControlWindow
  LDSelectionWindow
  LDVersionWindow
  LDConsoleWindow
  LDViewportWindow
  LDOutlinerWindow
  LDInspectorWindow
  LDEditorUI
)

add_subdirectory(LDEditor/EditorMain)
