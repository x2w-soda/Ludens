set(MODULE_NAME LDLua)
set(MODULE_TEST_NAME LDLuaTest)

set(MODULE_INCLUDE
    ${LUDENS_INCLUDE_DIR}/Ludens/Lua/LuaState.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Lua/LuaModule.h
    ${LUDENS_INCLUDE_DIR}/Ludens/Lua/LuaConfig.h
)

set(MODULE_LIB
    Lib/LuaState.cpp
    Lib/LuaModule.cpp
    Lib/LuaConfig.cpp
    Lib/LuaAST.h
    Lib/LuaAST.cpp
)

add_ludens_core_module(
    NAME    ${MODULE_NAME}
    INCLUDE ${MODULE_INCLUDE}
    LIB     ${MODULE_LIB}
)

target_include_directories(${MODULE_NAME} PUBLIC
    ${LUDENS_EXTRA_DIR}/luajit/src
)

if(WIN32)
target_link_directories(${MODULE_NAME} PUBLIC ${LUDENS_EXTRA_DIR}/luajit/prebuilt/win32)
set(LUAJIT_LIBS luajit lua51)
elseif(UNIX)
target_link_directories(${MODULE_NAME} PUBLIC ${LUDENS_EXTRA_DIR}/luajit/prebuilt/linux)
set(LUAJIT_LIBS luajit)
endif()

target_link_libraries(${MODULE_NAME} PUBLIC
    LDProfiler
    LDSystem
    LDLexer
    ${LUAJIT_LIBS}
)

set(MODULE_TEST
    Test/LuaTest.cpp
    Test/LuaTest.h
    Test/LuaStateTest.cpp
    Test/LuaErrorTest.cpp
    Test/LuaModuleTest.cpp
    Test/LuaConfigTest.cpp
    Test/LuaASTTest.cpp
    Test/LuaFFITest.cpp
)

add_ludens_core_module_test(
    TEST_NAME ${MODULE_TEST_NAME}
    TEST      ${MODULE_TEST}
)

target_link_libraries(${MODULE_TEST_NAME} PUBLIC
    ${MODULE_NAME}
    ${LUAJIT_LIBS}
    LDLog
    LDDSA
)

target_link_options(${MODULE_TEST_NAME} PRIVATE -rdynamic)
