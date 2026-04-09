set(MODULE_NAME LDAssetImportWindow)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/LudensEditor/AssetImportWindow/AssetImportWindow.h
)

set(MODULE_LIB
    Lib/AssetImportWindow.cpp
    Lib/AssetImport.h
    Lib/AssetImport.cpp
)

add_ludens_editor_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PRIVATE
    LDEditorContext
    LDEditorWidget
)
