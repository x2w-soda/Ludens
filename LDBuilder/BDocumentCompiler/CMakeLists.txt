set(MODULE_NAME LDBDocumentCompiler)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensBuilder/BDocumentCompiler/BDocumentCompiler.h
)

set(MODULE_LIB
	Lib/BDocumentCompiler.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
	LDCore
)