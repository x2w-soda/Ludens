set(MODULE_NAME LDDSA)
set(MODULE_TEST_NAME LDDSATest)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/DSA/Buffer.h
	${LUDENS_INCLUDE_DIR}/Ludens/DSA/HeapStorage.h
)

set(MODULE_LIB
	Lib/Buffer.cpp
)

set(MODULE_TEST
	Test/HeapStorageTest.h
	Test/DSATest.h
	Test/DSATest.cpp
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

add_executable(${MODULE_TEST_NAME}
    ${MODULE_TEST}
)

target_include_directories(${MODULE_TEST_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_SOURCE_DIR}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
	LDSystem
	LDDSA
)