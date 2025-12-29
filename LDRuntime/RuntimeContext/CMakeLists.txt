set(MODULE_NAME LDRuntimeContext)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensRuntime/RuntimeContext/RuntimeContext.h
)

set(MODULE_LIB
	Lib/RuntimeContext.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_RUNTIME_MODULE_FOLDER})

target_link_libraries(${MODULE_NAME} PRIVATE
	LDCoreLibs
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${LUDENS_SOURCE_DIR}
    ${LUDENS_INCLUDE_DIR}
)