set(MODULE_NAME LDDocumentCompiler)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensBuilder/DocumentCompiler/DocumentCompiler.h
)

set(MODULE_LIB
	Lib/DocumentCompiler.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
	LDCoreLibs
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_BUILDER_MODULE_FOLDER})