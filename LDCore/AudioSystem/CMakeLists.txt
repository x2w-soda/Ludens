set(MODULE_NAME LDAudioSystem)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/AudioSystem/AudioSystem.h
)

set(MODULE_LIB
	Lib/AudioSystem.cpp
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
