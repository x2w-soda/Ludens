set(MODULE_NAME LDSerial)
set(MODULE_TEST_NAME LDSerialTest)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/Serial/Serial.h
	${LUDENS_INCLUDE_DIR}/Ludens/Serial/Compress.h
	${LUDENS_INCLUDE_DIR}/Ludens/Serial/Format/JSON.h
)

set(MODULE_LIB
	Lib/Serial.cpp
	Lib/Compress.cpp
	Lib/Format/JSON.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
	LDDSA
	LDProfiler
	libzstd_static
	lz4_static
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${zstd_SOURCE_DIR}/lib
	${lz4_SOURCE_DIR}/lib
	${rapidjson_SOURCE_DIR}/include
)

set(MODULE_TEST
	Test/SerialTest.cpp
	Test/SerialJSONTest.cpp
	Test/TestCompression.h
)

add_executable(${MODULE_TEST_NAME}
	${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
	${MODULE_NAME}
	LDSystem
	LDJobSystem
	LDDSA
)

target_include_directories(${MODULE_TEST_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${LUDENS_SOURCE_DIR}
)
