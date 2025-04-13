cmake_minimum_required(VERSION 3.20)

project(Ludens LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build Environment

set(LUDENS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LUDENS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Include)
set(LUDENS_EXTRA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Extra)
set(LUDENS_BUILD_DIR ${CMAKE_BINARY_DIR})

message(STATUS "LUDENS SOURCE DIR  ${LUDENS_SOURCE_DIR}")
message(STATUS "LUDENS INCLUDE DIR ${LUDENS_INCLUDE_DIR}")
message(STATUS "LUDENS EXTRA DIR   ${LUDENS_EXTRA_DIR}")
message(STATUS "LUDENS BUILD DIR   ${LUDENS_BUILD_DIR}")

# Fetch Large Dependencies
# - smaller or single file dependencies are already in ${LUDENS_EXTRA_DIR}

include(FetchContent)

## Fetch GLFW

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.8
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Populate(glfw)
add_subdirectory(${glfw_SOURCE_DIR} glfw_build)
message(STATUS "LUDENS GLFW SOURCE DIR:     ${glfw_SOURCE_DIR}")

# Find Vulkan
# - require SPIRV-Cross for reflection
# - require glslang for SPIRV compilation
find_package(Vulkan REQUIRED SPIRV-Tools)

set(LUDENS_VULKAN_INCLUDE_DIR ${Vulkan_INCLUDE_DIR})
set(LUDENS_VULKAN_LIB_DIR     ${Vulkan_INCLUDE_DIR}/../Lib)
message(STATUS "LUDENS Vulkan INCLUDE DIR:  ${LUDENS_VULKAN_INCLUDE_DIR}")
message(STATUS "LUDENS Vulkan LIB DIR:      ${LUDENS_VULKAN_LIB_DIR}")

# Core Modules
# - strict dependency ordering
# - user facing API in ${LUDENS_INCLUDE_DIR}

add_subdirectory(Core/System)
add_subdirectory(Core/Serial)
add_subdirectory(Core/DSA)
add_subdirectory(Core/RenderBackend)
add_subdirectory(Core/Application)

add_library(LDCore INTERFACE)
target_link_directories(LDCore INTERFACE
  LDSystem
  LDSerial
  LDDSA
  LDRenderBackend
  LDApplication
)
