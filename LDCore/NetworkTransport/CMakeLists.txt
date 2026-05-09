set(MODULE_NAME LDNetworkTransport)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/NetworkTransport/NetworkTransportDef.h
    ${LUDENS_INCLUDE_DIR}/Ludens/NetworkTransport/TCPServer.h
    ${LUDENS_INCLUDE_DIR}/Ludens/NetworkTransport/TCPClient.h
)

set(MODULE_LIB
    Lib/TCPServer.cpp
    Lib/TCPClient.cpp
    Lib/TCPConnection.h
    Lib/TCPConnection.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDSystem
    LDJobSystem
)

target_include_directories(${MODULE_NAME} SYSTEM PRIVATE
    ${LUDENS_EXTRA_DIR}
    ${asio_SOURCE_DIR}/include
)
