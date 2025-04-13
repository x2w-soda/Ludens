set(MODULE_NAME LDDSA)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/DSA/Buffer.h
	${LUDENS_INCLUDE_DIR}/Ludens/DSA/Hash.h
)

set(MODULE_LIB
	Lib/Buffer.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PRIVATE
	LDSystem
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
)
