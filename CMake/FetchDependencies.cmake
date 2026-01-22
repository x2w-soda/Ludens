include(FetchContent)

## Fetch GLFW

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)
message(STATUS "LUDENS GLFW SOURCE DIR:     ${glfw_SOURCE_DIR}")

## Fetch readerwriterqueue (SPSC lock free queue)

FetchContent_Declare(
  readerwriterqueue
  GIT_REPOSITORY    https://github.com/cameron314/readerwriterqueue.git
  GIT_TAG           v1.0.7
)

FetchContent_MakeAvailable(readerwriterqueue)
message(STATUS "LUDENS readerwriterqueue SOURCE DIR:     ${readerwriterqueue_SOURCE_DIR}")

## Fetch ZLib

FetchContent_Declare(
  zlib
  GIT_REPOSITORY https://github.com/madler/zlib.git
  GIT_TAG 5a82f71ed1dfc0bec044d9702463dbdf84ea3b71
)
set(ZLIB_BUILD_STATIC   ON)
set(ZLIB_BUILD_SHARED   OFF)
FetchContent_MakeAvailable(zlib)
message(STATUS "LUDENS ZLIB SOURCE DIR:     ${zlib_SOURCE_DIR}")

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
FetchContent_MakeAvailable(libpng)
message(STATUS "LUDENS LIBPNG SOURCE DIR:   ${libpng_SOURCE_DIR}")
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
FetchContent_MakeAvailable(freetype)
message(STATUS "LUDENS freetype SOURCE DIR: ${freetype_SOURCE_DIR}")

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

## Fetch libsamplerate

FetchContent_Declare(
  libsamplerate
  GIT_REPOSITORY https://github.com/libsndfile/libsamplerate.git
  GIT_TAG 0.2.2
)
FetchContent_MakeAvailable(libsamplerate)

## Fetch ZStandard

FetchContent_Declare(
  zstd
  GIT_REPOSITORY https://github.com/facebook/zstd
  GIT_TAG v1.5.7
)
set(ZSTD_BUILD_SHARED  OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(zstd)
message(STATUS "LUDENS zstd SOURCE DIR:     ${zstd_SOURCE_DIR}")
add_subdirectory(${zstd_SOURCE_DIR}/build/cmake ${zstd_BINARY_DIR})

## Fetch LZ4

FetchContent_Declare(
  lz4
  GIT_REPOSITORY https://github.com/lz4/lz4
  GIT_TAG v1.10.0
)
set(LZ4_BUILD_CLI         OFF CACHE BOOL "" FORCE)
set(LZ4_BUILD_LEGACY_LZ4C OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(lz4)
message(STATUS "LUDENS lz4 SOURCE DIR:      ${lz4_SOURCE_DIR}")
add_subdirectory(${lz4_SOURCE_DIR}/build/cmake ${lz4_BINARY_DIR})

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
FetchContent_MakeAvailable(msdfgen)
message(STATUS "LUDENS msdfgen SOURCE DIR: ${msdfgen_SOURCE_DIR}")

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
FetchContent_MakeAvailable(msdfatlasgen)
message(STATUS "LUDENS msdf-atlas-gen SOURCE DIR: ${msdfgen_SOURCE_DIR}")

## Fetch Vulkan Memory Allocator

FetchContent_Declare(
  vma
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG v3.1.0
)
FetchContent_MakeAvailable(vma)
message(STATUS "LUDENS VMA SOURCE DIR:      ${vma_SOURCE_DIR}")

## Fetch tinygltf

FetchContent_Declare(
  tinygltf
  GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
  GIT_TAG v2.9.5
)

set(TINYGLTF_BUILD_LOADER_EXAMPLE     OFF CACHE BOOL "" FORCE)
set(TINYGLTF_BUILD_GL_EXAMPLES        OFF CACHE BOOL "" FORCE)
set(TINYGLTF_BUILD_VALIDATOR_EXAMPLE  OFF CACHE BOOL "" FORCE)
set(TINYGLTF_BUILD_BUILDER_EXAMPLE    OFF CACHE BOOL "" FORCE)
set(TINYGLTF_HEADER_ONLY              OFF CACHE BOOL "" FORCE)
set(TINYGLTF_INSTALL                  OFF CACHE BOOL "" FORCE)
set(TINYGLTF_INSTALL_VENDOR           OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(tinygltf)
message(STATUS "LUDENS tinygltf SOURCE DIR: ${tinygltf_SOURCE_DIR}")
target_link_libraries(tinygltf PUBLIC stb)

## Fetch MD4C

FetchContent_Declare(
  md4c
  GIT_REPOSITORY https://github.com/mity/md4c.git
  GIT_TAG release-0.5.2
)

set(BUILD_MD2HTML_EXECUTABLE   OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(md4c)
message(STATUS "LUDENS md4c SOURCE DIR:     ${md4c_SOURCE_DIR}")

## Fetch Toml11

FetchContent_Declare(
  toml11
  GIT_REPOSITORY https://github.com/ToruNiina/toml11.git
  GIT_TAG        v4.4.0
)
FetchContent_MakeAvailable(toml11)
message(STATUS "LUDENS Toml11 SOURCE DIR:    ${toml11_SOURCE_DIR}")

## Fetch Tracy Profiler

FetchContent_Declare(
  tracy
  GIT_REPOSITORY https://github.com/wolfpld/tracy.git
  GIT_TAG v0.11.1
)
FetchContent_MakeAvailable(tracy)
message(STATUS "LUDENS tracy SOURCE DIR:    ${tracy_SOURCE_DIR}")

## Fetch miniaudio.h

FetchContent_Declare(
  miniaudio
  GIT_REPOSITORY https://github.com/mackron/miniaudio.git
  GIT_TAG 0.11.23
)
FetchContent_MakeAvailable(miniaudio)
message(STATUS "LUDENS miniaudio SOURCE DIR:    ${miniaudio_SOURCE_DIR}")

## Fetch Khronos Texture

#FetchContent_Declare(
#  ktxsoftware
#  GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
#  GIT_TAG v4.4.0
#)
#set(KTX_FEATURE_DOC                OFF CACHE BOOL "" FORCE)
#set(KTX_FEATURE_JNI                OFF CACHE BOOL "" FORCE)
#set(KTX_FEATURE_PY                 OFF CACHE BOOL "" FORCE)
#set(KTX_FEATURE_TESTS              OFF CACHE BOOL "" FORCE)
#set(KTX_FEATURE_TOOLS              OFF CACHE BOOL "" FORCE)
#set(KTX_FEATURE_TOOLS_CTS          OFF CACHE BOOL "" FORCE)
#FetchContent_MakeAvailable(ktxsoftware)
#message(STATUS "LUDENS ktxsoftware SOURCE DIR:  ${ktxsoftware_SOURCE_DIR}")
