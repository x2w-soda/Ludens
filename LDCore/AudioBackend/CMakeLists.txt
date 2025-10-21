set(MODULE_NAME LDAudioBackend)
set(MODULE_TEST_NAME LDAudioBackendTest)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/AudioBackend/AudioBackend.h
	${LUDENS_INCLUDE_DIR}/Ludens/AudioBackend/MiniAudio.h
)

set(MODULE_LIB
	Lib/AudioBackend.cpp
	Lib/MiniAudio.cpp
	${miniaudio_SOURCE_DIR}/miniaudio.c
)

set(MODULE_TEST
	Test/AudioBackendTest.cpp
	Test/MiniAudioTest.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
	LDLog
	LDProfiler
	LDMedia
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${miniaudio_SOURCE_DIR}
)

add_executable(${MODULE_TEST_NAME}
	${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
	LDAudioBackend
)

target_include_directories(${MODULE_TEST_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${LUDENS_SOURCE_DIR}
)