set(MODULE_NAME LDDSA)
set(MODULE_TEST_NAME LDDSATest)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/DSA/IDCounter.h
	${LUDENS_INCLUDE_DIR}/Ludens/DSA/Buffer.h
	${LUDENS_INCLUDE_DIR}/Ludens/DSA/GapBuffer.h
	${LUDENS_INCLUDE_DIR}/Ludens/DSA/HeapStorage.h
	${LUDENS_INCLUDE_DIR}/Ludens/DSA/Stack.h
	${LUDENS_INCLUDE_DIR}/Ludens/DSA/Queue.h
	${LUDENS_INCLUDE_DIR}/Ludens/DSA/Vector.h
	${LUDENS_INCLUDE_DIR}/Ludens/DSA/String.h
	${LUDENS_INCLUDE_DIR}/Ludens/DSA/Observer.h
	${LUDENS_INCLUDE_DIR}/Ludens/DSA/Optional.h
	${LUDENS_INCLUDE_DIR}/Ludens/DSA/HashSet.h
	${LUDENS_INCLUDE_DIR}/Ludens/DSA/HashMap.h
	${LUDENS_INCLUDE_DIR}/Ludens/DSA/RectSplit.h
	${LUDENS_INCLUDE_DIR}/Ludens/DSA/Diagnostics.h
)

set(MODULE_LIB
	Lib/Buffer.cpp
	Lib/Diagnostics.cpp
)

set(MODULE_TEST
	Test/IDCounterTest.cpp
	Test/HeapStorageTest.h
	Test/GapBufferTest.cpp
	Test/VectorTest.h
	Test/StringTest.h
	Test/ObserverTest.cpp
	Test/RectSplitTest.cpp
	Test/DiagnosticsTest.cpp
	Test/DSATest.h
	Test/DSATest.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_link_libraries(${MODULE_NAME} PRIVATE
	LDSystem
)

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
)

add_executable(${MODULE_TEST_NAME}
    ${MODULE_TEST}
)

set_target_properties(${MODULE_TEST_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_include_directories(${MODULE_TEST_NAME} PRIVATE
    ${LUDENS_INCLUDE_DIR}
    ${LUDENS_SOURCE_DIR}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
	LDSystem
	LDDSA
)