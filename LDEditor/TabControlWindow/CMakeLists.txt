set(MODULE_NAME LDTabControlWindow)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensEditor/TabControlWindow/TabControlWindow.h
)

set(MODULE_LIB
	Lib/TabControlWindow.cpp
)

add_library(${MODULE_NAME}
	${MODULE_LIB}
	${MODULE_INCLUDE}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_EDITOR_MODULE_FOLDER})

target_link_libraries(${MODULE_NAME} PRIVATE
	LDCoreLibs
	LDEditorContext
    LDEditorWidget
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
)
