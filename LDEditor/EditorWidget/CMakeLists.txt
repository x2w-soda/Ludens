set(MODULE_NAME LDEditorWidget)

set(MODULE_LIB
	Lib/UITransformEditWidget.cpp
	Lib/UIAssetSlotWidget.cpp
	Lib/UIDropdownWindow.cpp
	Lib/UIVersionWindow.cpp
	Lib/UISelectWindow.cpp
	Lib/UIDraw.cpp
)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/UITransformEditWidget.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/UIAssetSlotWidget.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/UIDropdownWindow.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/UIVersionWindow.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/UISelectWindow.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorWidget/UIDraw.h
)

add_library(${MODULE_NAME}
	${MODULE_LIB}
	${MODULE_INCLUDE}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_EDITOR_MODULE_FOLDER})

target_link_libraries(${MODULE_NAME} PRIVATE
	LDCoreLibs
	LDEditorContext
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
)