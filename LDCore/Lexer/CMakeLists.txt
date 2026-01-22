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

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

set(MODULE_TEST
	Test/LexerTest.cpp
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
	${MODULE_NAME}
)