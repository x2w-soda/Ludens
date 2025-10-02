set(MODULE_NAME LDEditorContext)

set(MODULE_LIB
	Lib/EditorContext.cpp
	Lib/EditorSettings.cpp
	Lib/EditorIconAtlas.cpp
	Lib/EditStack.cpp
)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorContextEvent.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorContext.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorSettings.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorWindowObj.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorCallback.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditorIconAtlas.h
	${LUDENS_INCLUDE_DIR}/LudensEditor/EditorContext/EditStack.h
)

add_library(${MODULE_NAME}
	${MODULE_LIB}
	${MODULE_INCLUDE}
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDCoreLibs
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
)