set(MODULE_NAME LDMedia)
set(MODULE_TEST_NAME LDMediaTest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Font.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Model.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Bitmap.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Format/WAV.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Format/JSON.h
)

set(MODULE_LIB
    Lib/Font.cpp
    Lib/Model.cpp
    Lib/ModelObj.h
    Lib/Bitmap.cpp
    Lib/GlyphTable.h
    Lib/GlyphTable.cpp
    Lib/Format/TinygltfLoader.h
    Lib/Format/TinygltfLoader.cpp
    Lib/Format/WAV.cpp
    Lib/Format/JSON.cpp
)

set(MODULE_TEST
    Test/MediaTest.cpp
    Test/JSONTest.cpp
)

add_library(${MODULE_NAME}
    ${MODULE_LIB}
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

add_executable(${MODULE_TEST_NAME}
    ${MODULE_TEST}
)

target_include_directories(${MODULE_TEST_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_SOURCE_DIR}
)