set(MODULE_NAME LDAudioMixer)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/AudioMixer/AudioMixer.h
	${LUDENS_INCLUDE_DIR}/Ludens/AudioMixer/AudioBuffer.h
	${LUDENS_INCLUDE_DIR}/Ludens/AudioMixer/AudioCommand.h
	${LUDENS_INCLUDE_DIR}/Ludens/AudioMixer/AudioPlayback.h
)

set(MODULE_LIB
	Lib/AudioMixer.cpp
	Lib/AudioBuffer.cpp
	Lib/AudioCommand.cpp
	Lib/AudioPlayback.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
	LDAudioBackend
	LDDSP
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${readerwriterqueue_SOURCE_DIR}
)
