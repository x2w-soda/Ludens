set(MODULE_NAME LDMedia)
set(MODULE_TEST_NAME LDMediaTest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Font.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Model.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Bitmap.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/AudioData.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Win32Struct.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Format/WAV.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Format/MP3.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Format/ICO.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Format/PNG.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Format/BMP.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Format/MD.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Format/XML.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Format/JSON.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Format/TOML.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Format/GLTF.h
)

set(MODULE_LIB
    Lib/Font.cpp
    Lib/Model.cpp
    Lib/ModelObj.h
    Lib/Bitmap.cpp
    Lib/AudioDataObj.h
    Lib/AudioData.cpp
    Lib/GlyphTable.h
    Lib/GlyphTable.cpp
    Lib/Win32Struct.cpp
    Lib/Format/TinygltfLoader.h
    Lib/Format/TinygltfLoader.cpp
    Lib/Format/WAV.cpp
    Lib/Format/MP3.cpp
    Lib/Format/ICO.cpp
    Lib/Format/PNG.cpp
    Lib/Format/BMP.cpp
    Lib/Format/MD.cpp
    Lib/Format/XML.cpp
    Lib/Format/JSON.cpp
    Lib/Format/TOML.cpp
    Lib/Format/GLTF.cpp
    ${miniaudio_SOURCE_DIR}/miniaudio.c
)

set(MODULE_TEST
    Test/MediaTest.cpp
    Test/MDTest.cpp
    Test/XMLTest.cpp
    Test/JSONTest.cpp
    Test/TOMLTest.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    stb
    md4c
    tinygltf
    msdfgen::msdfgen
    msdf-atlas-gen
    LDProfiler
    LDSystem
    LDJobSystem
    LDLog
    LDSerial
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_EXTRA_DIR}
    ${LUDENS_EXTRA_DIR}/rapidjson/include
    ${msdfgen_SOURCE_DIR}
    ${tinygltf_SOURCE_DIR}
    ${md4c_SOURCE_DIR}/src
    ${miniaudio_SOURCE_DIR}
    ${toml11_SOURCE_DIR}/include
)

target_compile_definitions(${MODULE_NAME} PRIVATE
    RAPIDJSON_NOMEMBERITERATORCLASS
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
)
