#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Lexer/Lexer.h>
#include <cstddef>

namespace LD {

enum LuaTokenType
{
    LUA_TOK_EOF,
    // keywords
    LUA_TOK_IF,
    LUA_TOK_ELSEIF,
    LUA_TOK_ELSE,
    LUA_TOK_THEN,
    LUA_TOK_END,
    LUA_TOK_FUNCTION,
    LUA_TOK_RETURN,
    LUA_TOK_BREAK,
    LUA_TOK_NIL,
    LUA_TOK_TRUE,
    LUA_TOK_FALSE,
    LUA_TOK_AND,
    LUA_TOK_OR,
    LUA_TOK_NOT,
    // punctuators
    LUA_TOK_PLUS,
    LUA_TOK_MINUS,
    LUA_TOK_COMMA,
    LUA_TOK_MUL,
    LUA_TOK_DIV,
    LUA_TOK_LE,
    LUA_TOK_LT,
    LUA_TOK_EQ,
    LUA_TOK_GE,
    LUA_TOK_GT,
    // other
    LUA_TOK_SINGLE_LINE_COMMENT,
    LUA_TOKEN_TYPE_ENUM_COUNT,
};

using LuaToken = Token<LuaTokenType>;

enum LuaNodeType
{
    LUA_NODE_ROOT,
    LUA_NODE_RETURN,
    LUA_NODE_EXPR_LIST,
    LUA_NODE_LITERAL,
    LUA_NODE_TYPE_ENUM_COUNT,
};

struct LuaNode
{
    LuaNodeType type;
    LuaToken* tok;
    LuaNode* next;
    LuaNode* lch;
};

struct LuaParser : Handle<struct LuaParserObj>
{
    /// @brief Create lua AST parser.
    static LuaParser create();

    /// @brief Destroy lua AST parser.
    static void destroy(LuaParser parser);

    /// @brief Generate AST from Lua 5.1 source code.
    LuaNode* parse(const char* buf, size_t len, LuaToken** outTokens);
};

} // namespace LD
