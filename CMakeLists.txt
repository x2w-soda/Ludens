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

## Fetch ZLib

FetchContent_Declare(
  zlib
  GIT_REPOSITORY https://github.com/madler/zlib.git
  GIT_TAG 5a82f71ed1dfc0bec044d9702463dbdf84ea3b71
)
set(ZLIB_BUILD_STATIC   ON)
set(ZLIB_BUILD_SHARED   OFF)
FetchContent_Populate(zlib)
message(STATUS "LUDENS ZLIB SOURCE DIR:     ${zlib_SOURCE_DIR}")
add_subdirectory(${zlib_SOURCE_DIR})

set(ZLIB_LIBRARY       zlibstatic         CACHE INTERNAL "")
set(ZLIB_INCLUDE_DIR   ${zlib_SOURCE_DIR} CACHE INTERNAL "")
set(ZLIB_INCLUDE_DIRS  ${zlib_SOURCE_DIR} CACHE INTERNAL "")
find_package(ZLIB REQUIRED)

## Fetch Libpng

FetchContent_Declare(
  libpng
  GIT_REPOSITORY https://github.com/pnggroup/libpng.git
  GIT_TAG v1.6.37
)
set(PNG_STATIC   ON  CACHE BOOL "" FORCE)
set(PNG_SHARED   OFF CACHE BOOL "" FORCE)
set(PNG_TESTS    OFF CACHE BOOL "" FORCE)
FetchContent_Populate(libpng)
message(STATUS "LUDENS LIBPNG SOURCE DIR:   ${libpng_SOURCE_DIR}")
add_subdirectory(${libpng_SOURCE_DIR})
configure_file("${libpng_SOURCE_DIR}/scripts/pnglibconf.h.prebuilt" "${libpng_SOURCE_DIR}/pnglibconf.h" COPYONLY)

set(PNG_LIBRARY png_static                   CACHE INTERNAL "")
set(PNG_PNG_INCLUDE_DIR ${libpng_SOURCE_DIR} CACHE INTERNAL "")
set(PNG_INCLUDE_DIRS ${libpng_SOURCE_DIR}    CACHE INTERNAL "")
find_package(PNG REQUIRED)

## Fetch Freetype

FetchContent_Declare(
  freetype
  GIT_REPOSITORY https://github.com/freetype/freetype.git
  GIT_TAG VER-2-13-2
)
set(FT_DISABLE_BZIP2      ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ   ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI     ON CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL      ON CACHE BOOL "" FORCE)
FetchContent_Populate(freetype)
message(STATUS "LUDENS freetype SOURCE DIR: ${freetype_SOURCE_DIR}")
add_subdirectory(${freetype_SOURCE_DIR})

set(FREETYPE_LIBRARY freetype CACHE INTERNAL "")
set(FREETYPE_INCLUDE_DIR_ft2build "${freetype_INCLUDE_DIR}/include" CACHE INTERNAL "")
set(FREETYPE_INCLUDE_DIR_freetype2 "${freetype_INCLUDE_DIR}/include" CACHE INTERNAL "")
set(FREETYPE_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIR_ft2build} ${FREETYPE_INCLUDE_DIR_freetype2} CACHE INTERNAL "")
add_library(Freetype::Freetype INTERFACE IMPORTED)
set_target_properties(Freetype::Freetype PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${freetype_SOURCE_DIR}/include/freetype"
  INTERFACE_LINK_LIBRARIES freetype 
)
find_package(Freetype REQUIRED)

## Fetch msdfgen

FetchContent_Declare(
  msdfgen
  GIT_REPOSITORY https://github.com/Chlumsky/msdfgen.git
  GIT_TAG v1.12
)

set(MSDFGEN_USE_VCPKG       OFF CACHE BOOL "" FORCE)
set(MSDFGEN_CORE_ONLY       OFF CACHE BOOL "" FORCE)
set(MSDFGEN_USE_SKIA        OFF CACHE BOOL "" FORCE)
set(MSDFGEN_DISABLE_PNG     ON  CACHE BOOL "" FORCE)
set(MSDFGEN_DISABLE_SVG     ON  CACHE BOOL "" FORCE)
set(MSDFGEN_DYNAMIC_RUNTIME ON  CACHE BOOL "" FORCE)
FetchContent_Populate(msdfgen)
message(STATUS "LUDENS msdfgen SOURCE DIR: ${msdfgen_SOURCE_DIR}")
add_subdirectory(${msdfgen_SOURCE_DIR})


## Fetch msdf atlas gen

FetchContent_Declare(
  msdfatlasgen
  GIT_REPOSITORY https://github.com/Chlumsky/msdf-atlas-gen.git
  GIT_TAG v1.3
)

set(MSDF_ATLAS_USE_VCPKG          OFF CACHE BOOL "" FORCE)
set(MSDF_ATLAS_MSDFGEN_EXTERNAL   ON  CACHE BOOL "" FORCE)
set(MSDF_ATLAS_NO_ARTERY_FONT     ON  CACHE BOOL "" FORCE)
set(MSDF_ATLAS_DYNAMIC_RUNTIME    ON  CACHE BOOL "" FORCE)
FetchContent_Populate(msdfatlasgen)
message(STATUS "LUDENS msdf-atlas-gen SOURCE DIR: ${msdfgen_SOURCE_DIR}")
add_subdirectory(${msdfatlasgen_SOURCE_DIR})

# Find Vulkan
# - require SPIRV-Cross for reflection
# - require glslang for SPIRV compilation
find_package(Vulkan REQUIRED SPIRV-Tools)

set(LUDENS_VULKAN_INCLUDE_DIR ${Vulkan_INCLUDE_DIR})
set(LUDENS_VULKAN_LIB_DIR     ${Vulkan_INCLUDE_DIR}/../Lib)
message(STATUS "LUDENS Vulkan INCLUDE DIR:  ${LUDENS_VULKAN_INCLUDE_DIR}")
message(STATUS "LUDENS Vulkan LIB DIR:      ${LUDENS_VULKAN_LIB_DIR}")

# build stb

add_subdirectory(${LUDENS_EXTRA_DIR}/stb)

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
