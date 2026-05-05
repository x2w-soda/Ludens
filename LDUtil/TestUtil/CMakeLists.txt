set(MODULE_NAME LDTestUtil)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/LudensUtil/TestUtil/TestUtil.h
)

set(MODULE_LIB
	Lib/TestUtil.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${LUDENS_SOURCE_DIR}
)
