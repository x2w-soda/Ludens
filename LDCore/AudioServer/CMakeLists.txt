set(MODULE_NAME LDAudioServer)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/AudioServer/AudioServer.h
)

set(MODULE_LIB
	Lib/AudioServer.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
	LDAudioBackend
	LDAudioMixer
)
