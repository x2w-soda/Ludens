set(MODULE_NAME LDMedia)

set(MODULE_INCLUDE
    "${LUDENS_INCLUDE_DIR}/Ludens/Media/Font.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/Media/Model.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/Media/Bitmap.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/Media/Parser/WAV.h"
)

add_library(${MODULE_NAME}
    Lib/Font.cpp
    Lib/Model.cpp
    Lib/ModelObj.h
    Lib/Bitmap.cpp
    Lib/GlyphTable.h
    Lib/GlyphTable.cpp
    Lib/Parser/TinygltfLoader.h
    Lib/Parser/TinygltfLoader.cpp
    Lib/Parser/WAV.cpp
)

target_link_libraries(${MODULE_NAME} PUBLIC
    stb
    tinygltf
    msdfgen::msdfgen
    msdf-atlas-gen
    LDSystem
    LDSerial
    LDProfiler
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${msdfgen_SOURCE_DIR}
    ${tinygltf_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_EXTRA_DIR}
)
