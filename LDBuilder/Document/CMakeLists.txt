set(MODULE_NAME LDDocument)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/LudensBuilder/Document/Document.h
	${LUDENS_INCLUDE_DIR}/LudensBuilder/Document/DocumentBuilder.h
)

set(MODULE_LIB
    Lib/Document.cpp
	Lib/DocumentBuilder.cpp
)

add_ludens_builder_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)