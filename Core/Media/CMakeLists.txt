set(MODULE_NAME LDMedia)

set(MODULE_INCLUDE
    "${LUDENS_INCLUDE_DIR}/Ludens/Media/Font.h"
    "${LUDENS_INCLUDE_DIR}/Ludens/Media/Bitmap.h"
)

add_library(${MODULE_NAME}
    Lib/Font.cpp
    Lib/Bitmap.cpp
)

target_link_libraries(${MODULE_NAME} PUBLIC
    stb
    msdfgen::msdfgen
    msdf-atlas-gen
    LDSystem
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${msdfgen_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_EXTRA_DIR}
)