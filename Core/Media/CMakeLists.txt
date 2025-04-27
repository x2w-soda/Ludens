set(MODULE_NAME LDMedia)

set(MODULE_INCLUDE
    "${LUDENS_INCLUDE_DIR}/Ludens/Media/Font.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/Media/Model.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/Media/Bitmap.h"
)

add_library(${MODULE_NAME}
    Lib/Font.cpp
    Lib/Model.cpp
    Lib/ModelObj.h
    Lib/Bitmap.cpp
    Lib/TinygltfLoader.h
    Lib/TinygltfLoader.cpp
)

target_link_libraries(${MODULE_NAME} PUBLIC
    stb
    tinygltf
    msdfgen::msdfgen
    msdf-atlas-gen
    LDSystem
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${msdfgen_SOURCE_DIR}
    ${tinygltf_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_EXTRA_DIR}
)