set(MODULE_NAME LDNetworkMessage)
set(MODULE_TEST_NAME LDNetworkMessageTest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/NetworkMessage/JSONRPCDef.h
    ${LUDENS_INCLUDE_DIR}/Ludens/NetworkMessage/JSONRPC.h
)

set(MODULE_LIB
    Lib/JSONRPC.cpp
)

set(MODULE_TEST
    Test/JSONRPCTest.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDNetworkTransport
    LDSerial
)

target_include_directories(${MODULE_NAME} SYSTEM PRIVATE
    ${LUDENS_EXTRA_DIR}
    ${asio_SOURCE_DIR}/include
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
)