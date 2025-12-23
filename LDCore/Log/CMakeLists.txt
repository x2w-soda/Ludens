set(MODULE_NAME LDLog)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/Log/Log.h
)

set(MODULE_LIB
	Lib/Log.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_link_libraries(${MODULE_NAME} PRIVATE
	LDSystem
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
)
