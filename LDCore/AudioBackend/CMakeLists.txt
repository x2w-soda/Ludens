set(MODULE_NAME LDAudioBackend)
set(MODULE_TEST_NAME LDAudioBackendTest)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/AudioBackend/AudioBackend.h
	${LUDENS_INCLUDE_DIR}/Ludens/AudioBackend/MiniAudio.h
)

set(MODULE_LIB
	Lib/AudioBackend.cpp
	Lib/MiniAudio.cpp
)

set(MODULE_TEST
	Test/AudioBackendTest.cpp
	Test/MiniAudioTest.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
	LDLog
	LDProfiler
	LDMedia
)

target_include_directories(${MODULE_NAME} PRIVATE
	${miniaudio_SOURCE_DIR}
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
	LDAudioBackend
)
