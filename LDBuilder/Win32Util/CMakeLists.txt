set(MODULE_NAME LDWin32Util)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensBuilder/Win32Util/Win32Util.h
)

set(MODULE_LIB
	Lib/Win32Util.cpp
)

add_ludens_builder_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)