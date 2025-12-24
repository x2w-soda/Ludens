set(MODULE_NAME LDEvent)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/Event/Event.h
)

set(MODULE_LIB
	Lib/Event.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_link_libraries(${MODULE_NAME} PRIVATE
	LDSerial
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
)
