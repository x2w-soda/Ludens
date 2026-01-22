set(MODULE_NAME LDAudioMixer)
set(MODULE_TEST_NAME LDAudioMixerTest)
set(MODULE_SANDBOX_NAME LDAudioMixerSandbox)

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

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
	LDAudioBackend
	LDDSP
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${readerwriterqueue_SOURCE_DIR}
)

set(MODULE_TEST
	Test/AudioMixerTest.h
	Test/AudioMixerTest.cpp
	Test/AudioMixerReadbackTest.cpp
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
	${MODULE_NAME}
	LDLudensLFS
)

set(MODULE_SANDBOX
	Sandbox/AudioMixerSandbox.cpp
)

add_executable(${MODULE_SANDBOX_NAME}
	${MODULE_SANDBOX}
)

set_target_properties(${MODULE_SANDBOX_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_link_libraries(${MODULE_SANDBOX_NAME} PRIVATE
	${MODULE_NAME}
	LDWindowRegistry
	LDLudensLFS
)

target_include_directories(${MODULE_SANDBOX_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${LUDENS_SOURCE_DIR}
)
