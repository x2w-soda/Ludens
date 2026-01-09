#include <Ludens/Memory/Memory.h>
#include <Ludens/Profiler/Profiler.h>

#include "LuaAST.h"

namespace LD {

static const char sLuaSLComment[] = "--";

// clang-format off
static LexerMatchRule<LuaTokenType> sMatchRules[] = {
    {LUA_TOK_IF,       "if",       2},
    {LUA_TOK_ELSEIF,   "elseif",   6},
    {LUA_TOK_ELSE,     "else",     4},
    {LUA_TOK_THEN,     "then",     4},
    {LUA_TOK_END,      "end",      3},
    {LUA_TOK_FUNCTION, "function", 8},
    {LUA_TOK_RETURN,   "return",   6},
    {LUA_TOK_BREAK,    "break",    5},
    {LUA_TOK_NIL,      "nil",      3},
    {LUA_TOK_TRUE,     "true",     4},
    {LUA_TOK_FALSE,    "false",    5},
    {LUA_TOK_AND,      "ands",     4},
    {LUA_TOK_OR,       "or",       2},
    {LUA_TOK_NOT,      "not",      3},
    {LUA_TOK_PLUS,     "+",        1},
    {LUA_TOK_MINUS,    "-",        1},
    {LUA_TOK_COMMA,    ",",        1},
    {LUA_TOK_MUL,      "*",        1},
    {LUA_TOK_DIV,      "/",        1},
    {LUA_TOK_LE,       "<=",       2},
    {LUA_TOK_LT,       "<",        1},
    {LUA_TOK_EQ,       "=",        1},
    {LUA_TOK_GE,       ">=",       2},
    {LUA_TOK_GT,       ">",        1},
};
// clang-format on

static inline bool tok_consume(LuaToken** tok, LuaTokenType type)
{
    if ((*tok)->type != type)
        return false;

    (*tok) = (*tok)->next;
    return true;
}

class LuaParserObj
{
public:
    LuaParserObj();
    ~LuaParserObj();

    /// @brief Get token stream from most recent parse_root.
    LuaToken* get_tokens()
    {
        return mTokens;
    }

    LuaNode* parse_root(const char* buf, size_t len);

private:
    LuaNode* new_node(LuaNodeType type, LuaToken* token);
    LuaNode* parse_stmt(LuaToken* now);
    LuaNode* parse_expr_list(LuaToken* now);
    LuaNode* parse_expr(LuaToken* now);

private:
    Lexer<LuaTokenType> mLexer;
    LinearAllocator mNodeLA;
    LuaToken* mTokens = nullptr;
    LuaToken* mNow = nullptr;
    LuaNode* mRoot = nullptr;
};

LuaParserObj::LuaParserObj()
{
    LexerInfo<LuaTokenType> lexerI{};
    lexerI.matchRules = sMatchRules;
    lexerI.matchRuleCount = sizeof(sMatchRules) / sizeof(*sMatchRules);
    lexerI.memoryUsage = MEMORY_USAGE_LUA;
    lexerI.singleLineComment = sLuaSLComment;
    lexerI.singleLineCommentToken = LUA_TOK_SINGLE_LINE_COMMENT;
    mLexer.startup(lexerI);
    mTokens = nullptr;
    mRoot = nullptr;

    LinearAllocatorInfo laI{};
    laI.capacity = sizeof(LuaNode) * 256;
    laI.usage = MEMORY_USAGE_LUA;
    laI.isMultiPage = true;
    mNodeLA = LinearAllocator::create(laI);
}

LuaParserObj::~LuaParserObj()
{
    LinearAllocator::destroy(mNodeLA);
    mLexer.cleanup();
}

LuaNode* LuaParserObj::new_node(LuaNodeType type, LuaToken* token)
{
    LuaNode* node = (LuaNode*)mNodeLA.allocate(sizeof(LuaNode));

    node->lch = nullptr;
    node->tok = token;
    node->next = nullptr;
    node->type = type;

    return node;
}

LuaNode* LuaParserObj::parse_root(const char* buf, size_t len)
{
    mNow = mTokens = mLexer.process((const uint8_t*)buf, len);

    if (!mTokens)
        return nullptr;

    LuaNode dummy = {.next = NULL};
    mRoot = new_node(LUA_NODE_ROOT, mNow);
    LuaNode* stmt = &dummy;

    while (mNow->type != LUA_TOK_EOF)
    {
        stmt = stmt->next = parse_stmt(mNow);

        if (!stmt)
            break;
    }

    mRoot->lch = dummy.next;

    return mRoot;
}

// stmt = "return" expr_list?
LuaNode* LuaParserObj::parse_stmt(LuaToken* now)
{
    LuaToken* old = now;

    if (!tok_consume(&now, LUA_TOK_RETURN))
        return nullptr;

    mNow = now;

    LuaNode* ret = new_node(LUA_NODE_RETURN, nullptr);
    ret->lch = parse_expr_list(now);

    return ret;
}

// expr_list = expr ("," expr)*
LuaNode* LuaParserObj::parse_expr_list(LuaToken* now)
{
    LuaToken* old = now;
    LuaNode dummy = {.next = nullptr};
    LuaNode* expr = &dummy;

    expr = expr->next = parse_expr(now);
    if (!expr)
    {
        mNow = old;
        return nullptr;
    }

    now = mNow;

    while (tok_consume(&now, LUA_TOK_COMMA))
    {
        expr = expr->next = parse_expr(now);
        if (!expr)
        {
            mNow = old;
            return nullptr;
        }
    }

    LuaNode* list = new_node(LUA_NODE_EXPR_LIST, nullptr);
    list->lch = dummy.next;

    mNow = now;
    return list;
}

// expr = "nil" |
//        "true" |
//        "false"
LuaNode* LuaParserObj::parse_expr(LuaToken* now)
{
    LuaNode* expr = nullptr;

    switch (now->type)
    {
    case LUA_TOK_NIL:
    case LUA_TOK_TRUE:
    case LUA_TOK_FALSE:
        expr = new_node(LUA_NODE_LITERAL, now);
        mNow = now->next;
        break;
    default:
        break;
    }

    return expr;
}

//
// PUBLIC API
//

LuaParser LuaParser::create()
{
    LuaParserObj* obj = heap_new<LuaParserObj>(MEMORY_USAGE_LUA);

    return LuaParser(obj);
}

void LuaParser::destroy(LuaParser parser)
{
    LuaParserObj* obj = parser.unwrap();

    heap_delete<LuaParserObj>(obj);
}

LuaNode* LuaParser::parse(const char* buf, size_t len, LuaToken** outTokens)
{
    LD_PROFILE_SCOPE;

    if (!buf || len == 0)
        return nullptr;

    LuaNode* root = mObj->parse_root(buf, len);

    if (outTokens)
        *outTokens = mObj->get_tokens();

    return root;
}

} // namespace LD
