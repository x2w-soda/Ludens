set(MODULE_NAME LDSerial)
set(MODULE_TEST_NAME LDSerialTest)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/Serial/Serial.h
	${LUDENS_INCLUDE_DIR}/Ludens/Serial/Packet.h
	${LUDENS_INCLUDE_DIR}/Ludens/Serial/Compress.h
	${LUDENS_INCLUDE_DIR}/Ludens/Serial/Endianness.h
)

set(MODULE_LIB
	Lib/Serial.cpp
	Lib/Packet.cpp
	Lib/Compress.cpp
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
)

set(MODULE_TEST
	Test/SerialTest.cpp
	Test/PacketTest.cpp
	Test/EndiannessTest.cpp
	Test/CompressionTest.cpp
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
