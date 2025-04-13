set(MODULE_NAME LDSerial)
set(MODULE_TEST_NAME LDSerialTest)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/Serial/Serial.h
)

set(MODULE_LIB
	Lib/Serial.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
)

set(MODULE_TEST
	Test/SerialTest.cpp
)

add_executable(${MODULE_TEST_NAME}
	${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
	${MODULE_NAME}
	LDSystem
	LDDSA
)

target_include_directories(${MODULE_TEST_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${LUDENS_SOURCE_DIR}
)
