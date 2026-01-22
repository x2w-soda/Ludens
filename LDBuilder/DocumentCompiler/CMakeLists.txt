set(MODULE_NAME LDDocumentCompiler)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensBuilder/DocumentCompiler/DocumentCompiler.h
)

set(MODULE_LIB
	Lib/DocumentCompiler.cpp
)

add_ludens_builder_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)