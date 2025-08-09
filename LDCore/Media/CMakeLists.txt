set(MODULE_NAME LDMedia)
set(MODULE_TEST_NAME LDMediaTest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Font.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Model.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Bitmap.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Format/WAV.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Format/MD.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Media/Format/XML.h
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
    Lib/Format/MD.cpp
    Lib/Format/XML.cpp
    Lib/Format/JSON.cpp
)

set(MODULE_TEST
    Test/MediaTest.cpp
    Test/MDTest.cpp
    Test/XMLTest.cpp
    Test/JSONTest.cpp
)

add_library(${MODULE_NAME}
    ${MODULE_LIB}
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
    ${msdfgen_SOURCE_DIR}
    ${tinygltf_SOURCE_DIR}
    ${md4c_SOURCE_DIR}/src
    ${rapidjson_SOURCE_DIR}/include
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

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    LDMedia
)
