set(MODULE_NAME LDSerial)
set(MODULE_TEST_NAME LDSerialTest)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/Serial/Value.h
	${LUDENS_INCLUDE_DIR}/Ludens/Serial/Serial.h
	${LUDENS_INCLUDE_DIR}/Ludens/Serial/Packet.h
	${LUDENS_INCLUDE_DIR}/Ludens/Serial/Compress.h
	${LUDENS_INCLUDE_DIR}/Ludens/Serial/Endianness.h
)

set(MODULE_LIB
	Lib/Value.cpp
	Lib/Serial.cpp
	Lib/Packet.cpp
	Lib/Compress.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
	LDDSA
	LDProfiler
	libzstd_static
	lz4_static
)

target_include_directories(${MODULE_NAME} PRIVATE
	${zstd_SOURCE_DIR}/lib
	${lz4_SOURCE_DIR}/lib
)

set(MODULE_TEST
	Test/SerialTest.cpp
	Test/PacketTest.cpp
	Test/EndiannessTest.cpp
	Test/CompressionTest.cpp
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
	${MODULE_NAME}
	LDSystem
	LDJobSystem
	LDDSA
)
