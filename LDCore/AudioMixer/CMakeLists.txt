set(MODULE_NAME LDAudioMixer)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/AudioMixer/AudioMixer.h
	${LUDENS_INCLUDE_DIR}/Ludens/AudioMixer/AudioBuffer.h
	${LUDENS_INCLUDE_DIR}/Ludens/AudioMixer/AudioCommand.h
	${LUDENS_INCLUDE_DIR}/Ludens/AudioMixer/AudioPlayback.h
	${LUDENS_INCLUDE_DIR}/Ludens/AudioMixer/AudioEffect.h
	${LUDENS_INCLUDE_DIR}/Ludens/AudioMixer/AudioEffectInfo.h
	${LUDENS_INCLUDE_DIR}/Ludens/AudioMixer/Effect/AudioEffectLowPassFilter.h
	${LUDENS_INCLUDE_DIR}/Ludens/AudioMixer/Effect/AudioEffectHighPassFilter.h
)

set(MODULE_LIB
	Lib/AudioMixer.cpp
	Lib/AudioBuffer.cpp
	Lib/AudioCommand.cpp
	Lib/AudioPlaybackObj.h
	Lib/AudioPlayback.cpp
	Lib/AudioEffect.cpp
	Lib/Effect/AudioEffectLowPassFilter.cpp
	Lib/Effect/AudioEffectHighPassFilter.cpp
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
