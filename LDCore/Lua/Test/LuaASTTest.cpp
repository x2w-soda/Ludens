#include "LuaTest.h"
#include <Extra/doctest/doctest.h>
#include <LDCore/Lua/Lib/LuaAST.h>

using namespace LD;

struct
{
    LuaTokenType type;
    const char* str;
} sLuaTokDebugTable[] = {
    {LUA_TOK_EOF, "LUA_TOK_EOF"},
    {LUA_TOK_IF, "LUA_TOK_IF"},
    {LUA_TOK_ELSEIF, "LUA_TOK_ELSEIF"},
    {LUA_TOK_ELSE, "LUA_TOK_ELSE"},
    {LUA_TOK_THEN, "LUA_TOK_THEN"},
    {LUA_TOK_END, "LUA_TOK_END"},
    {LUA_TOK_FUNCTION, "LUA_TOK_FUNCTION"},
    {LUA_TOK_RETURN, "LUA_TOK_RETURN"},
    {LUA_TOK_BREAK, "LUA_TOK_BREAK"},
    {LUA_TOK_NIL, "LUA_TOK_NIL"},
    {LUA_TOK_TRUE, "LUA_TOK_TRUE"},
    {LUA_TOK_FALSE, "LUA_TOK_FALSE"},
    {LUA_TOK_AND, "LUA_TOK_AND"},
    {LUA_TOK_OR, "LUA_TOK_OR"},
    {LUA_TOK_NOT, "LUA_TOK_NOT"},
    {LUA_TOK_PLUS, "LUA_TOK_PLUS"},
    {LUA_TOK_MINUS, "LUA_TOK_MINUS"},
    {LUA_TOK_COMMA, "LUA_TOK_COMMA"},
    {LUA_TOK_MUL, "LUA_TOK_MUL"},
    {LUA_TOK_DIV, "LUA_TOK_DIV"},
    {LUA_TOK_LE, "LUA_TOK_LE"},
    {LUA_TOK_LT, "LUA_TOK_LT"},
    {LUA_TOK_EQ, "LUA_TOK_EQ"},
    {LUA_TOK_GE, "LUA_TOK_GE"},
    {LUA_TOK_GT, "LUA_TOK_GT"},
    {LUA_TOK_SINGLE_LINE_COMMENT, "LUA_TOK_SINGLE_LINE_COMMENT"},
};

static_assert(sizeof(sLuaTokDebugTable) / sizeof(*sLuaTokDebugTable) == (size_t)LUA_TOKEN_TYPE_ENUM_COUNT);

static void print_lua_tok(LuaToken* tok)
{
    printf("%s\n", sLuaTokDebugTable[(int)tok->type].str);
}

static void print_lua_tokens(LuaToken* list)
{
    for (LuaToken* tok = list; tok; tok = tok->next)
        print_lua_tok(tok);
}

// TODO: match AST against expected JSON?