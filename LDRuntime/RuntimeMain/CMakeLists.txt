set(MODULE_NAME LDRuntime)

set(MODULE_LIB
	Lib/RuntimeMain.cpp
	Lib/RuntimeApplication.h
	Lib/RuntimeApplication.cpp
)

add_executable(${MODULE_NAME}
	${MODULE_LIB}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_RUNTIME_MODULE_FOLDER})

target_link_libraries(${MODULE_NAME} PRIVATE
	LDCoreLibs
	LDRuntimeContext
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
)