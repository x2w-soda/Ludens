set(MODULE_NAME LDAudioServer)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/AudioServer/AudioServer.h
)

set(MODULE_LIB
	Lib/AudioServer.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
	LDAudioBackend
	LDAudioMixer
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
)
