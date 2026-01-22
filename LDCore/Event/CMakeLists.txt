set(MODULE_NAME LDEvent)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/Event/Event.h
	${LUDENS_INCLUDE_DIR}/Ludens/Event/WindowEvent.h
)

set(MODULE_LIB
	Lib/Event.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDSerial
)
