set(MODULE_NAME LDEditor)

set(MODULE_LIB
	Lib/EditorApplication.h
	Lib/EditorApplication.cpp
	Lib/EditorMain.cpp
)

add_executable(${MODULE_NAME}
	${MODULE_LIB}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_EDITOR_MODULE_FOLDER})

target_link_libraries(${MODULE_NAME} PRIVATE
	LDCoreLibs
	LDBuilderLibs
	LDEditorLibs
	LDLudensLFS # TODO: embed required media instead of fetching from submodule
)

target_link_options(${MODULE_NAME} PRIVATE -rdynamic)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
)