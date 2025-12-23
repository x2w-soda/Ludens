set(MODULE_NAME LDLexer)
set(MODULE_TEST_NAME LDLexerTest)

set(MODULE_INCLUDE
	${LUDENS_INCLUDE_DIR}/Ludens/Lexer/Unicode.h
	${LUDENS_INCLUDE_DIR}/Ludens/Lexer/Token.h
	${LUDENS_INCLUDE_DIR}/Ludens/Lexer/Lexer.h
)

set(MODULE_LIB
	Lib/Unicode.cpp
)

add_library(${MODULE_NAME}
	${MODULE_INCLUDE}
	${MODULE_LIB}
)

set_target_properties(${MODULE_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_include_directories(${MODULE_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
)

add_executable(${MODULE_TEST_NAME}
	Test/LexerTest.cpp
)

set_target_properties(${MODULE_TEST_NAME} PROPERTIES FOLDER ${LD_CORE_MODULE_FOLDER})

target_include_directories(${MODULE_TEST_NAME} PRIVATE
	${LUDENS_INCLUDE_DIR}
	${LUDENS_SOURCE_DIR}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
	${MODULE_NAME}
)