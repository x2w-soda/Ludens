set(MODULE_NAME LDText)
set(MODULE_TEST_NAME LDTextTest)

set(MODULE_TEST
    Test/TextTest.cpp
)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/Text/TextBuffer.h
)

set(MODULE_LIB
    Lib/TextBuffer.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_link_libraries(${MODULE_NAME} PUBLIC
    LDSystem
    LDDSA
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PRIVATE
    ${MODULE_NAME}
)