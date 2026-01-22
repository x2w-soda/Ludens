set(MODULE_NAME LDLog)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/Log/Log.h
)

set(MODULE_LIB
	Lib/Log.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDSystem
)
