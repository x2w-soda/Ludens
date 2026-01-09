#include <Ludens/Header/Assert.h>
#include <Ludens/Header/Bitwise.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Memory/Allocator.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/LDShaderParser.h>
#include <cctype>
#include <cstring>
#include <unordered_set>

#define TOKEN_PAGE_SIZE 512
#define NODE_PAGE_SIZE 512

namespace LD {

enum LDShaderTokenFlag : uint32_t
{
    LDSTF_STORAGE_QUALIFIER_BIT = LD_BIT(0),
    LDSTF_TYPE_SPECIFIER_BIT = LD_BIT(1),
    LDSTF_ASSIGNMENT_BIT = LD_BIT(2),
    LDSTF_POSTFIX_BIT = LD_BIT(3),
    LDSTF_UNARY_BIT = LD_BIT(4),
};

// clang-format off
struct
{
    const char* cstr;
    size_t len;
    LDShaderTokenType type;
    const uint32_t flags;
} sTokenTable[] = {
    { nullptr,  0, LDS_TOK_EOF,            0 },
    { nullptr,  0, LDS_TOK_IDENT,          0 },
    { nullptr,  0, LDS_TOK_INT_CONSTANT,   0 },
    { nullptr,  0, LDS_TOK_UINT_CONSTANT,  0 },
    { nullptr,  0, LDS_TOK_BOOL_CONSTANT,  0 },
    // keyword entries
    { "while",         5,  LDS_TOK_WHILE,         0 },
    { "break",         5,  LDS_TOK_BREAK,         0 },
    { "continue",      8,  LDS_TOK_CONTINUE,      0 },
    { "do",            2,  LDS_TOK_DO,            0 },
    { "else",          4,  LDS_TOK_ELSE,          0 },
    { "for",           3,  LDS_TOK_FOR,           0 },
    { "if",            2,  LDS_TOK_IF,            0 },
    { "discard",       7,  LDS_TOK_DISCARD,       0 },
    { "return",        6,  LDS_TOK_RETURN,        0 },
    { "switch",        6,  LDS_TOK_SWITCH,        0 },
    { "case",          4,  LDS_TOK_CASE,          0 },
    { "default",       7,  LDS_TOK_DEFAULT,       0 },
    { "subroutine",    10, LDS_TOK_SUBROUTINE,    0 },
    { "const",         5,  LDS_TOK_CONST,         LDSTF_STORAGE_QUALIFIER_BIT },
    { "struct",        6,  LDS_TOK_STRUCT,        LDSTF_TYPE_SPECIFIER_BIT },
    { "void",          4,  LDS_TOK_VOID,          LDSTF_TYPE_SPECIFIER_BIT },
    { "float",         5,  LDS_TOK_FLOAT,         LDSTF_TYPE_SPECIFIER_BIT },
    { "double",        6,  LDS_TOK_DOUBLE,        LDSTF_TYPE_SPECIFIER_BIT },
    { "int",           3,  LDS_TOK_INT,           LDSTF_TYPE_SPECIFIER_BIT },
    { "uint",          4,  LDS_TOK_UINT,          LDSTF_TYPE_SPECIFIER_BIT },
    { "bool",          4,  LDS_TOK_BOOL,          LDSTF_TYPE_SPECIFIER_BIT },
    { "vec2",          4,  LDS_TOK_VEC2,          LDSTF_TYPE_SPECIFIER_BIT },
    { "vec3",          4,  LDS_TOK_VEC3,          LDSTF_TYPE_SPECIFIER_BIT },
    { "vec4",          4,  LDS_TOK_VEC4,          LDSTF_TYPE_SPECIFIER_BIT },
    { "dvec2",         5,  LDS_TOK_DVEC2,         LDSTF_TYPE_SPECIFIER_BIT },
    { "dvec3",         5,  LDS_TOK_DVEC3,         LDSTF_TYPE_SPECIFIER_BIT },
    { "dvec4",         5,  LDS_TOK_DVEC4,         LDSTF_TYPE_SPECIFIER_BIT },
    { "bvec2",         5,  LDS_TOK_BVEC2,         LDSTF_TYPE_SPECIFIER_BIT },
    { "bvec3",         5,  LDS_TOK_BVEC3,         LDSTF_TYPE_SPECIFIER_BIT },
    { "bvec4",         5,  LDS_TOK_BVEC4,         LDSTF_TYPE_SPECIFIER_BIT },
    { "ivec2",         5,  LDS_TOK_IVEC2,         LDSTF_TYPE_SPECIFIER_BIT },
    { "ivec3",         5,  LDS_TOK_IVEC3,         LDSTF_TYPE_SPECIFIER_BIT },
    { "ivec4",         5,  LDS_TOK_IVEC4,         LDSTF_TYPE_SPECIFIER_BIT },
    { "uvec2",         5,  LDS_TOK_UVEC2,         LDSTF_TYPE_SPECIFIER_BIT },
    { "uvec3",         5,  LDS_TOK_UVEC3,         LDSTF_TYPE_SPECIFIER_BIT },
    { "uvec4",         5,  LDS_TOK_UVEC4,         LDSTF_TYPE_SPECIFIER_BIT },
    { "mat2",          4,  LDS_TOK_MAT2,          LDSTF_TYPE_SPECIFIER_BIT },
    { "mat3",          4,  LDS_TOK_MAT3,          LDSTF_TYPE_SPECIFIER_BIT },
    { "mat4",          4,  LDS_TOK_MAT4,          LDSTF_TYPE_SPECIFIER_BIT },
    { "dmat2",         5,  LDS_TOK_DMAT2,         LDSTF_TYPE_SPECIFIER_BIT },
    { "dmat3",         5,  LDS_TOK_DMAT3,         LDSTF_TYPE_SPECIFIER_BIT },
    { "dmat4",         5,  LDS_TOK_DMAT4,         LDSTF_TYPE_SPECIFIER_BIT },
    // sampler types
    { "sampler1D",              9,   LDS_TOK_SAMPLER1D,              LDSTF_TYPE_SPECIFIER_BIT },
    { "sampler1DArray",         14,  LDS_TOK_SAMPLER1DARRAY,         LDSTF_TYPE_SPECIFIER_BIT },
    { "sampler1DArrayShadow",   20,  LDS_TOK_SAMPLER1DARRAYSHADOW,   LDSTF_TYPE_SPECIFIER_BIT },
    { "sampler1DShadow",        15,  LDS_TOK_SAMPLER1DSHADOW,        LDSTF_TYPE_SPECIFIER_BIT },
    { "sampler2D",              9,   LDS_TOK_SAMPLER2D,              LDSTF_TYPE_SPECIFIER_BIT },
    { "sampler2DArray",         14,  LDS_TOK_SAMPLER2DARRAY,         LDSTF_TYPE_SPECIFIER_BIT },
    { "sampler2DArrayShadow",   20,  LDS_TOK_SAMPLER2DARRAYSHADOW,   LDSTF_TYPE_SPECIFIER_BIT },
    { "sampler2DShadow",        15,  LDS_TOK_SAMPLER2DSHADOW,        LDSTF_TYPE_SPECIFIER_BIT },
    { "sampler3D",              9,   LDS_TOK_SAMPLER3D,              LDSTF_TYPE_SPECIFIER_BIT },
    { "samplerCube",            11,  LDS_TOK_SAMPLERCUBE,            LDSTF_TYPE_SPECIFIER_BIT },
    { "samplerCubeArray",       16,  LDS_TOK_SAMPLERCUBEARRAY,       LDSTF_TYPE_SPECIFIER_BIT },
    { "samplerCubeArrayShadow", 22,  LDS_TOK_SAMPLERCUBEARRAYSHADOW, LDSTF_TYPE_SPECIFIER_BIT },
    { "samplerCubeShadow",      17,  LDS_TOK_SAMPLERCUBESHADOW,      LDSTF_TYPE_SPECIFIER_BIT },
    // image types
    { "image1D",                7,   LDS_TOK_IMAGE1D,                LDSTF_TYPE_SPECIFIER_BIT },
    { "image1DArray",           12,  LDS_TOK_IMAGE1DARRAY,           LDSTF_TYPE_SPECIFIER_BIT },
    { "image2D",                7,   LDS_TOK_IMAGE2D,                LDSTF_TYPE_SPECIFIER_BIT },
    { "image2DArray",           12,  LDS_TOK_IMAGE2DARRAY,           LDSTF_TYPE_SPECIFIER_BIT },
    { "image3D",                7,   LDS_TOK_IMAGE3D,                LDSTF_TYPE_SPECIFIER_BIT },
    { "imageCube",              9,   LDS_TOK_IMAGECUBE,              LDSTF_TYPE_SPECIFIER_BIT },
    { "imageCubeArray",         14,  LDS_TOK_IMAGECUBEARRAY,         LDSTF_TYPE_SPECIFIER_BIT },
    { "iimage1D",               8,   LDS_TOK_IIMAGE1D,               LDSTF_TYPE_SPECIFIER_BIT },
    { "iimage1DArray",          13,  LDS_TOK_IIMAGE1DARRAY,          LDSTF_TYPE_SPECIFIER_BIT },
    { "iimage2D",               8,   LDS_TOK_IIMAGE2D,               LDSTF_TYPE_SPECIFIER_BIT },
    { "iimage2DArray",          13,  LDS_TOK_IIMAGE2DARRAY,          LDSTF_TYPE_SPECIFIER_BIT },
    { "iimage3D",               8,   LDS_TOK_IIMAGE3D,               LDSTF_TYPE_SPECIFIER_BIT },
    { "iimageCube",             10,  LDS_TOK_IIMAGECUBE,             LDSTF_TYPE_SPECIFIER_BIT },
    { "iimageCubeArray",        15,  LDS_TOK_IIMAGECUBEARRAY,        LDSTF_TYPE_SPECIFIER_BIT },
    { "uimage1D",               8,   LDS_TOK_UIMAGE1D,               LDSTF_TYPE_SPECIFIER_BIT },
    { "uimage1DArray",          13,  LDS_TOK_UIMAGE1DARRAY,          LDSTF_TYPE_SPECIFIER_BIT },
    { "uimage2D",               8,   LDS_TOK_UIMAGE2D,               LDSTF_TYPE_SPECIFIER_BIT },
    { "uimage2DArray",          13,  LDS_TOK_UIMAGE2DARRAY,          LDSTF_TYPE_SPECIFIER_BIT },
    { "uimage3D",               8,   LDS_TOK_UIMAGE3D,               LDSTF_TYPE_SPECIFIER_BIT },
    { "uimageCube",             10,  LDS_TOK_UIMAGECUBE,             LDSTF_TYPE_SPECIFIER_BIT },
    { "uimageCubeArray",        15,  LDS_TOK_UIMAGECUBEARRAY,        LDSTF_TYPE_SPECIFIER_BIT },
    // storage qualifiers
    { "inout",         5,  LDS_TOK_INOUT,         LDSTF_STORAGE_QUALIFIER_BIT },
    { "in",            2,  LDS_TOK_IN,            LDSTF_STORAGE_QUALIFIER_BIT },
    { "out",           3,  LDS_TOK_OUT,           LDSTF_STORAGE_QUALIFIER_BIT },
    { "uniform",       7,  LDS_TOK_UNIFORM,       LDSTF_STORAGE_QUALIFIER_BIT },
    { "patch",         5,  LDS_TOK_PATCH,         LDSTF_STORAGE_QUALIFIER_BIT },
    { "sample",        6,  LDS_TOK_SAMPLE,        LDSTF_STORAGE_QUALIFIER_BIT },
    { "buffer",        6,  LDS_TOK_BUFFER,        LDSTF_STORAGE_QUALIFIER_BIT },
    { "shared",        6,  LDS_TOK_SHARED,        LDSTF_STORAGE_QUALIFIER_BIT },
    { "coherent",      8,  LDS_TOK_COHERENT,      LDSTF_STORAGE_QUALIFIER_BIT },
    { "volatile",      8,  LDS_TOK_VOLATILE,      LDSTF_STORAGE_QUALIFIER_BIT },
    { "restrict",      8,  LDS_TOK_RESTRICT,      LDSTF_STORAGE_QUALIFIER_BIT },
    { "readonly",      8,  LDS_TOK_READONLY,      LDSTF_STORAGE_QUALIFIER_BIT },
    { "writeonly",     9,  LDS_TOK_WRITEONLY,     LDSTF_STORAGE_QUALIFIER_BIT },
    { "noperspective", 13, LDS_TOK_NOPERSPECTIVE, 0 },
    { "flat",          4,  LDS_TOK_FLAT,          0 },
    { "smooth",        6,  LDS_TOK_SMOOTH,        0 },
    { "layout",        6,  LDS_TOK_LAYOUT,        0 },
    // punctuator entries
    { "<<",     2, LDS_TOK_LEFT_OP,       0 },
    { ">>",     2, LDS_TOK_RIGHT_OP,      0 },
    { "++",     2, LDS_TOK_INC_OP,        LDSTF_UNARY_BIT | LDSTF_POSTFIX_BIT },
    { "--",     2, LDS_TOK_DEC_OP,        LDSTF_UNARY_BIT | LDSTF_POSTFIX_BIT },
    { "<=",     2, LDS_TOK_LE_OP,         0 },
    { ">=",     2, LDS_TOK_GE_OP,         0 },
    { "==",     2, LDS_TOK_EQ_OP,         0 },
    { "!=",     2, LDS_TOK_NE_OP,         0 },
    { "&&",     2, LDS_TOK_AND_OP,        0 },
    { "||",     2, LDS_TOK_OR_OP,         0 },
    { "^^",     2, LDS_TOK_XOR_OP,        0 },
    { "+=",     2, LDS_TOK_ADD_ASSIGN,    LDSTF_ASSIGNMENT_BIT },
    { "-=",     2, LDS_TOK_SUB_ASSIGN,    LDSTF_ASSIGNMENT_BIT },
    { "*=",     2, LDS_TOK_MUL_ASSIGN,    LDSTF_ASSIGNMENT_BIT },
    { "/=",     2, LDS_TOK_DIV_ASSIGN,    LDSTF_ASSIGNMENT_BIT },
    { "%=",     2, LDS_TOK_MOD_ASSIGN,    LDSTF_ASSIGNMENT_BIT },
    { "<<=",    3, LDS_TOK_LEFT_ASSIGN,   LDSTF_ASSIGNMENT_BIT },
    { ">>=",    3, LDS_TOK_RIGHT_ASSIGN,  LDSTF_ASSIGNMENT_BIT },
    { "&=",     2, LDS_TOK_AND_ASSIGN,    LDSTF_ASSIGNMENT_BIT },
    { "^=",     2, LDS_TOK_XOR_ASSIGN,    LDSTF_ASSIGNMENT_BIT },
    { "|=",     2, LDS_TOK_OR_ASSIGN,     LDSTF_ASSIGNMENT_BIT },
    { "(",      1, LDS_TOK_LEFT_PAREN,    LDSTF_POSTFIX_BIT /* call */ },
    { ")",      1, LDS_TOK_RIGHT_PAREN,   0 },
    { "[",      1, LDS_TOK_LEFT_BRACKET,  LDSTF_POSTFIX_BIT /* indexing */ },
    { "]",      1, LDS_TOK_RIGHT_BRACKET, 0 },
    { "{",      1, LDS_TOK_LEFT_BRACE,    0 },
    { "}",      1, LDS_TOK_RIGHT_BRACE,   0 },
    { ".",      1, LDS_TOK_DOT,           LDSTF_POSTFIX_BIT /* struct member */ },
    { ",",      1, LDS_TOK_COMMA,         0 },
    { ":",      1, LDS_TOK_COLON,         0 },
    { "=",      1, LDS_TOK_EQUAL,         LDSTF_ASSIGNMENT_BIT },
    { ";",      1, LDS_TOK_SEMICOLON,     0 },
    { "!",      1, LDS_TOK_BANG,          LDSTF_UNARY_BIT },
    { "-",      1, LDS_TOK_DASH,          LDSTF_UNARY_BIT },
    { "~",      1, LDS_TOK_TILDE,         LDSTF_UNARY_BIT },
    { "+",      1, LDS_TOK_PLUS,          LDSTF_UNARY_BIT },
    { "*",      1, LDS_TOK_STAR,          0 },
    { "/",      1, LDS_TOK_SLASH,         0 },
    { "%",      1, LDS_TOK_PERCENT,       0 },
    { "<",      1, LDS_TOK_LEFT_ANGLE,    0 },
    { ">",      1, LDS_TOK_RIGHT_ANGLE,   0 },
    { "|",      1, LDS_TOK_VERTICAL_BAR,  0 },
    { "^",      1, LDS_TOK_CARET,         0 },
    { "&",      1, LDS_TOK_AMPERSAND,     0 },
    { "?",      1, LDS_TOK_QUESTION,      0 },
};

struct {
    const char* cstr;
    LDShaderNodeType type;
} sNodeTable[] = {
    { "translation_unit",    LDS_NODE_TRANSLATION_UNIT, },
    { "single_decl",         LDS_NODE_SINGLE_DECL, },
    { "fn_prototype",        LDS_NODE_FN_PROTOTYPE, },
    { "fn_param_decl",       LDS_NODE_FN_PARAM_DECL, },
    { "fn_definition",       LDS_NODE_FN_DEFINITION, },
    { "empty_stmt",          LDS_NODE_EMPTY_STMT, },
    { "compound_stmt",       LDS_NODE_COMPOUND_STMT, },
    { "if_stmt",             LDS_NODE_IF_STMT, },
    { "for_stmt",            LDS_NODE_FOR_STMT, },
    { "while_stmt",          LDS_NODE_WHILE_STMT, },
    { "switch_stmt",         LDS_NODE_SWITCH_STMT, },
    { "switch_case",         LDS_NODE_SWITCH_CASE, },
    { "expr_stmt",           LDS_NODE_EXPR_STMT, },
    { "control_flow_stmt",   LDS_NODE_CONTROL_FLOW_STMT, },
    { "type_specifier",      LDS_NODE_TYPE_SPECIFIER, },
    { "type_qualifier",      LDS_NODE_TYPE_QUALIFIER, },
    { "struct_specifier",    LDS_NODE_STRUCT_SPECIFIER, },
    { "struct_decl",         LDS_NODE_STRUCT_DECL, },
    { "struct_member",       LDS_NODE_STRUCT_MEMBER, },
    { "array_specifier",     LDS_NODE_ARRAY_SPECIFIER, },
    { "layout_qualifier",    LDS_NODE_LAYOUT_QUALIFIER, },
    { "layout_qualifier_id", LDS_NODE_LAYOUT_QUALIFIER_ID, },
    { "storage_qualifier",   LDS_NODE_STORAGE_QUALIFIER, },
    { "initializer",         LDS_NODE_INITIALIZER, },
    { "assignment",          LDS_NODE_ASSIGNMENT, },
    { "conditional",         LDS_NODE_CONDITIONAL, },
    { "logical_or",          LDS_NODE_LOGICAL_OR, },
    { "logical_xor",         LDS_NODE_LOGICAL_XOR, },
    { "logical_and",         LDS_NODE_LOGICAL_AND, },
    { "bitwise_or",          LDS_NODE_BITWISE_OR, },
    { "bitwise_xor",         LDS_NODE_BITWISE_XOR, },
    { "bitwise_and",         LDS_NODE_BITWISE_AND, },
    { "equal",               LDS_NODE_EQUAL, },
    { "relational",          LDS_NODE_RELATIONAL, },
    { "shift",               LDS_NODE_SHIFT, },
    { "add",                 LDS_NODE_ADD, },
    { "mul",                 LDS_NODE_MUL, },
    { "unary",               LDS_NODE_UNARY, },
    { "index",               LDS_NODE_INDEX, },
    { "postfix",             LDS_NODE_POSTFIX, },
    { "call",                LDS_NODE_CALL, },
    { "var",                 LDS_NODE_VAR, },
    { "constant",            LDS_NODE_CONSTANT, },
};
// clang-format on

static constexpr int sTokenTableKeywordBegin = (int)LDS_TOK_WHILE;
static constexpr int sTokenTableKeywordEnd = (int)LDS_TOK_LAYOUT;
static constexpr int sTokenTablePunctBegin = (int)LDS_TOK_LEFT_OP;
static constexpr int sTokenTablePunctEnd = (int)LDS_TOK_QUESTION;

static_assert(sizeof(sTokenTable) / sizeof(*sTokenTable) == LDS_TOK_ENUM_COUNT);
static_assert(sizeof(sNodeTable) / sizeof(*sNodeTable) == LDS_NODE_ENUM_COUNT);

static inline bool consume(LDShaderToken** tok, LDShaderTokenType tokType)
{
    if ((*tok)->type != tokType)
        return false;

    *tok = (*tok)->next;
    return true;
}

static bool in_range(char c, const char* ranges)
{
    for (char min, max; *ranges; ranges += 2)
    {
        min = ranges[0];
        max = ranges[1];

        if (min <= c && c <= max)
            return true;
    }

    return false;
}

static bool is_keyword_tok(const char* str, int strLen, int* tokLen, LDShaderTokenType* tokType)
{
    *tokLen = 0;
    *tokType = LDS_TOK_EOF;

    for (int i = sTokenTableKeywordBegin; i <= sTokenTableKeywordEnd; i++)
    {
        size_t matchLen = (size_t)sTokenTable[i].len;
        if (!strncmp(str, sTokenTable[i].cstr, matchLen))
        {
            *tokLen = (int)matchLen;
            *tokType = sTokenTable[i].type;
            return true;
        }
    }

    return false;
}

static bool is_punct_tok(const char* str, int strLen, int* tokLen, LDShaderTokenType* tokType)
{
    *tokLen = 0;
    *tokType = LDS_TOK_EOF;

    for (int i = sTokenTablePunctBegin; i <= sTokenTablePunctEnd; i++)
    {
        size_t matchLen = (size_t)sTokenTable[i].len;
        if (!strncmp(str, sTokenTable[i].cstr, matchLen))
        {
            *tokLen = (int)matchLen;
            *tokType = sTokenTable[i].type;
            return true;
        }
    }

    return false;
}

static bool is_ident_tok(const char* str, int strLen, int* tokLen)
{
    static const char sIdent1Ranges[] = {'a', 'z', 'A', 'Z', 0};
    static const char sIdent2Ranges[] = {'0', '9', '_', '_', 0};

    *tokLen = 0;

    if (!in_range(*str, sIdent1Ranges))
        return false;

    for (int i = 1; i < strLen; i++)
    {
        char c = str[i];

        if (isspace(c) || !(in_range(c, sIdent1Ranges) || in_range(c, sIdent2Ranges)))
        {
            *tokLen = i;
            return true;
        }
    }

    *tokLen = strLen;
    return true;
}

static bool is_constant_tok(const char* str, int strLen, int* tokLen, LDShaderTokenType* tokType)
{
    *tokLen = 0;
    *tokType = LDS_TOK_EOF;

    if (!strncmp(str, "true", 4))
    {
        *tokLen = 4;
        *tokType = LDS_TOK_BOOL_CONSTANT;
        return true;
    }

    if (!strncmp(str, "false", 5))
    {
        *tokLen = 5;
        *tokType = LDS_TOK_BOOL_CONSTANT;
        return true;
    }

    // TODO: support hex and octal prefix?
    if (isdigit(*str))
    {
        int i;
        for (i = 0; i < strLen && isdigit(str[i]); i++)
            ;
        LDShaderTokenType constantType = LDS_TOK_INT_CONSTANT;

        if (i < strLen && (str[i] == 'u' || str[i] == 'U'))
        {
            i++;
            constantType = LDS_TOK_UINT_CONSTANT;
        }

        *tokLen = i;
        *tokType = constantType;
        return true;

        // TODO: LDS_TOK_FLOAT_CONSTANT,
    }

    return false;
}

static inline bool is_storage_qualifier_tok(const LDShaderToken* tok)
{
    return (sTokenTable[(int)tok->type].flags & LDSTF_STORAGE_QUALIFIER_BIT);
}

static inline bool is_type_specifier_tok(const LDShaderToken* tok)
{
    return (sTokenTable[(int)tok->type].flags & LDSTF_TYPE_SPECIFIER_BIT);
}

static inline bool is_assignment_tok(const LDShaderToken* tok)
{
    return (sTokenTable[(int)tok->type].flags & LDSTF_ASSIGNMENT_BIT);
}

static inline bool is_postfix_tok(const LDShaderToken* tok)
{
    return (sTokenTable[(int)tok->type].flags & LDSTF_POSTFIX_BIT);
}

static inline bool is_unary_tok(const LDShaderToken* tok)
{
    return (sTokenTable[(int)tok->type].flags & LDSTF_UNARY_BIT);
}

static void recursive_traverse(LDShaderNode* root, LDShaderAST::TraverseFn onTraverse, int depth, void* user)
{
    if (!root)
        return;

    onTraverse(root, depth, user);

    ++depth;
    recursive_traverse(root->init, onTraverse, depth, user);
    recursive_traverse(root->cond, onTraverse, depth, user);
    recursive_traverse(root->lch, onTraverse, depth, user);
    recursive_traverse(root->rch, onTraverse, depth, user);
    --depth;

    recursive_traverse(root->next, onTraverse, depth, user);
}

static void print_node_fn(const LDShaderNode* root, int depth, void* user)
{
    std::string& str = *(std::string*)user;

    str += std::string(depth * 2, ' ');
    str += sNodeTable[(int)root->type].cstr;

    if (root->tok)
    {
        str += ' ';
        str += std::string(root->tok->pos, root->tok->len);
    }

    str += '\n';
}

struct LDShaderASTObj
{
    LDShaderASTObj();
    LDShaderASTObj(const LDShaderASTObj&) = delete;
    ~LDShaderASTObj();

    LDShaderASTObj& operator=(const LDShaderASTObj&) = delete;

    LDShaderNode* alloc_node(LDShaderNodeType type);
    LDShaderNode* alloc_node_lch(LDShaderNodeType type, LDShaderNode* lch);

    PoolAllocator nodePA; /// node pool allocator
    LDShaderNode* root;   /// root node, should always be translation unit
};

/// @brief Contains a lexer and a recursive-descent parser for ldshader source code.
class LDShaderParserObj
{
public:
    LDShaderParserObj();
    LDShaderParserObj(const LDShaderParserObj&) = delete;
    ~LDShaderParserObj();

    LDShaderParserObj& operator=(const LDShaderParserObj&) = delete;

    LDShaderAST parse(const char* str, size_t strLen, LDShaderType type);

private:
    LDShaderToken* alloc_token(LDShaderTokenType type, const char* pos, int len);

    void tokenize(const char* str, size_t strLen);

    bool is_struct_ident(LDShaderToken* tok);

    // high level parsing rules

    LDShaderNode* parse_translation_unit(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_decl(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_fn_prototype(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_fn_param_decl(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_single_decl(LDShaderToken** stream, LDShaderToken* now);

    // statement parsing rules

    LDShaderNode* parse_stmt(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_compound_stmt(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_if_stmt(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_for_stmt(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_while_stmt(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_switch_stmt(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_switch_case(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_expr_stmt(LDShaderToken** stream, LDShaderToken* now);

    // type parsing rules

    LDShaderNode* parse_full_type(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_type_qualifier(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_type_specifier(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_struct_specifier(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_struct_decl(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_struct_member(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_array_specifier(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_single_type_qualifier(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_storage_qualifer(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_layout_qualifier(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_layout_qualifier_id(LDShaderToken** stream, LDShaderToken* now);

    // expression parsing rules

    LDShaderNode* parse_initializer(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_initializer_list(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_expr(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_assignment(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_conditional(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_logical_or(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_logical_xor(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_logical_and(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_bitwise_or(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_bitwise_xor(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_bitwise_and(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_equal(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_relational(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_shift(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_add(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_mul(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_unary(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_postfix(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_postfix_expr(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_call(LDShaderToken** stream, LDShaderToken* now);
    LDShaderNode* parse_primary(LDShaderToken** stream, LDShaderToken* now);

private:
    PoolAllocator mTokenPA;                  /// token pool allocator
    LDShaderToken* mTokens;                  /// token linked list
    LDShaderASTObj* mAST;                    /// current AST being parsed
    std::string mSource;                     /// ldshader source copy
    std::unordered_set<Hash32> mStructIdent; /// hashes of user-defined struct names
    int mLine;                               /// parser current line in source code
    int mCol;                                /// parser current column in source code
};

LDShaderASTObj::LDShaderASTObj()
    : root(nullptr)
{
    PoolAllocatorInfo paI;
    paI.usage = MEMORY_USAGE_MISC;
    paI.blockSize = sizeof(LDShaderNode);
    paI.pageSize = NODE_PAGE_SIZE;
    paI.isMultiPage = true;

    nodePA = PoolAllocator::create(paI);
}

LDShaderASTObj::~LDShaderASTObj()
{
    PoolAllocator::destroy(nodePA);
}

LDShaderNode* LDShaderASTObj::alloc_node(LDShaderNodeType type)
{
    LDShaderNode* node = (LDShaderNode*)nodePA.allocate();
    node->lch = nullptr;
    node->rch = nullptr;
    node->init = nullptr;
    node->cond = nullptr;
    node->next = nullptr;
    node->tok = nullptr;
    node->type = type;

    return node;
}

LDShaderNode* LDShaderASTObj::alloc_node_lch(LDShaderNodeType type, LDShaderNode* lch)
{
    LDShaderNode* node = alloc_node(type);
    node->lch = lch;

    return node;
}

LDShaderParserObj::LDShaderParserObj()
    : mCol(0), mLine(0), mTokens(nullptr), mAST(nullptr)
{
    PoolAllocatorInfo paI;
    paI.usage = MEMORY_USAGE_MISC;
    paI.blockSize = sizeof(LDShaderToken);
    paI.pageSize = TOKEN_PAGE_SIZE;
    paI.isMultiPage = true;

    mTokenPA = PoolAllocator::create(paI);
}

LDShaderParserObj::~LDShaderParserObj()
{
    // TODO: delete all ASTs created from this parser
    if (mAST)
        heap_delete<LDShaderASTObj>(mAST);

    PoolAllocator::destroy(mTokenPA);
}

LDShaderToken* LDShaderParserObj::alloc_token(LDShaderTokenType type, const char* pos, int len)
{
    LDShaderToken* tok = (LDShaderToken*)mTokenPA.allocate();
    tok->next = nullptr;
    tok->type = type;
    tok->col = mCol;
    tok->line = mLine;
    tok->pos = pos;
    tok->len = len;

    return tok;
}

void LDShaderParserObj::tokenize(const char* str, size_t strLen)
{
    const char* end = str + strLen;
    LDShaderToken dummy = {.next = NULL};
    LDShaderToken* tok = &dummy;
    LDShaderTokenType tokType;
    int tokLen;

    while (str < end)
    {
        // whitespace
        while (str < end && isspace(*str))
        {
            mCol++;

            if (*str++ == '\n')
            {
                mCol = 0;
                mLine++;
            }
        }

        if (str >= end)
            break;
        strLen = end - str;

        // single line comment
        if (strLen >= 2 && str[0] == '/' && str[1] == '/')
        {
            str += 2;

            while (str < end && *str++ != '\n')
                ;

            mCol = 0;
            mLine++;

            if (str >= end)
                break;

            strLen = end - str;
        }

        // multi line comment
        if (strLen >= 2 && str[0] == '/' && str[1] == '*')
        {
            str += 2;
            mCol += 2;

            while (str < end - 1 && !(str[0] == '*' && str[1] == '/'))
            {
                mCol++;

                if (*str++ == '\n')
                {
                    mCol = 0;
                    mLine++;
                }
            }

            if (str >= end)
                break;

            str += 2;
            mCol += 2;
            strLen = end - str;
        }

        if (is_keyword_tok(str, strLen, &tokLen, &tokType))
        {
            tok = tok->next = alloc_token(tokType, str, tokLen);
            str += tokLen;
            mCol += tokLen;
            continue;
        }

        if (is_punct_tok(str, strLen, &tokLen, &tokType))
        {
            tok = tok->next = alloc_token(tokType, str, tokLen);
            str += tokLen;
            mCol += tokLen;
            continue;
        }

        if (is_constant_tok(str, strLen, &tokLen, &tokType))
        {
            tok = tok->next = alloc_token(tokType, str, tokLen);
            str += tokLen;
            mCol += tokLen;
            continue;
        }

        if (is_ident_tok(str, strLen, &tokLen))
        {
            tok = tok->next = alloc_token(LDS_TOK_IDENT, str, tokLen);
            str += tokLen;
            mCol += tokLen;
            continue;
        }

        str++;
    }

    tok = tok->next = alloc_token(LDS_TOK_EOF, nullptr, 0);
    mTokens = dummy.next;
}

bool LDShaderParserObj::is_struct_ident(LDShaderToken* tok)
{
    Hash32 hash = hash32_FNV_1a(tok->pos, tok->len);

    return mStructIdent.contains(hash);
}

/// translation_unit = (decl)*
LDShaderNode* LDShaderParserObj::parse_translation_unit(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode dummy = {.next = nullptr};
    LDShaderNode* decl = &dummy;
    LDShaderNode* root = mAST->alloc_node(LDS_NODE_TRANSLATION_UNIT);

    while (now->type != LDS_TOK_EOF)
    {
        decl = decl->next = parse_decl(&now, now);
        LD_ASSERT(decl);
    }

    root->lch = dummy.next;
    return root;
}

/// decl = single_decl (EQUAL initializer) SEMICOLON |
///        fn_prototype SEMICOLON |
///        fn_prototype compound_stmt
LDShaderNode* LDShaderParserObj::parse_decl(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root;
    LDShaderToken* old = now;

    if ((root = parse_single_decl(&now, now)))
    {
        if (consume(&now, LDS_TOK_EQUAL))
            root->init = parse_initializer(&now, now);

        if (consume(&now, LDS_TOK_SEMICOLON))
        {
            *stream = now;
            return root;
        }
    }

    now = old;
    if ((root = parse_fn_prototype(&now, now)))
    {
        if (consume(&now, LDS_TOK_SEMICOLON))
        {
            // function declaration
            *stream = now;
            return root;
        }

        // NOTE: Technically (fn_prototype compound_stmt) is a function definition and does
        //       not classify as a declaration. We are handling function definition here
        //       to avoid some backtracking via a single look-ahead token, comparing ';' vs '{'
        if (now->type == LDS_TOK_LEFT_BRACE)
        {
            // function definition, prototype stored as left child, body stored as right child
            root = mAST->alloc_node_lch(LDS_NODE_FN_DEFINITION, root);
            root->rch = parse_compound_stmt(&now, now);
            *stream = now;
            return root;
        }

        *stream = old;
        return nullptr;
    }

    *stream = old;
    return nullptr;
}

/// single_decl = full_type (IDENT array_speicifer?)? |
///               type_qualifier (IDENT struct_decl)?
LDShaderNode* LDShaderParserObj::parse_single_decl(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderToken* old = now;
    LDShaderNode* root = parse_full_type(&now, now);

    if (root)
    {
        root = mAST->alloc_node_lch(LDS_NODE_SINGLE_DECL, root);

        if (now->type == LDS_TOK_IDENT)
        {
            root->tok = now; // single decl identifier
            now = now->next;

            if (now->type == LDS_TOK_LEFT_BRACKET)
            {
                root->rch = parse_array_specifier(&now, now);
            }
        }

        *stream = now;
        return root;
    }

    now = old;
    root = parse_type_qualifier(&now, now);

    if (root)
    {
        root = mAST->alloc_node_lch(LDS_NODE_SINGLE_DECL, root);

        if (now->type == LDS_TOK_IDENT && now->next->type == LDS_TOK_LEFT_BRACE)
        {
            root->tok = now; // single decl identifier
            now = now->next;

            root->rch = parse_struct_decl(&now, now);
        }

        *stream = now;
        return root;
    }

    *stream = old;
    return nullptr;
}

/// fn_prototype = full_type IDENT LEFT_PAREN (fn_param_decl (COMMA fn_param_decl)*)? RIGHT_PAREN
LDShaderNode* LDShaderParserObj::parse_fn_prototype(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderToken* old = now;
    LDShaderNode* fullType = parse_full_type(&now, now);

    if (!fullType)
    {
        *stream = old;
        return nullptr;
    }

    LD_ASSERT(now->type == LDS_TOK_IDENT);
    LDShaderNode* root = mAST->alloc_node_lch(LDS_NODE_FN_PROTOTYPE, fullType);
    root->tok = now; // function name identifier
    now = now->next;

    if (!consume(&now, LDS_TOK_LEFT_PAREN))
    {
        *stream = old;
        return nullptr;
    }

    if (now->type != LDS_TOK_RIGHT_PAREN)
    {
        LDShaderNode* firstParam = parse_fn_param_decl(&now, now);
        LDShaderNode* lastParam = firstParam;

        if (!firstParam)
        {
            // TODO: error invalid param
            return nullptr;
        }

        while (consume(&now, LDS_TOK_COMMA))
        {
            lastParam = lastParam->next = parse_fn_param_decl(&now, now);

            if (!lastParam)
            {
                // TODO: error invalid param
                return nullptr;
            }
        }

        // function parameters linked list stored as right child
        root->rch = firstParam;
    }

    if (!consume(&now, LDS_TOK_RIGHT_PAREN))
        return nullptr;

    *stream = now;
    return root;
}

/// fn_param_decl = full_type (IDENT array_specifier?)?
/// @note this rule should cover parameter_declarator and parameter_declaration in the spec
LDShaderNode* LDShaderParserObj::parse_fn_param_decl(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderToken* old = now;
    LDShaderNode* paramType = parse_full_type(&now, now);

    if (!paramType)
    {
        *stream = old;
        return nullptr;
    }

    LDShaderNode* root = mAST->alloc_node(LDS_NODE_FN_PARAM_DECL);
    root->lch = paramType;

    if (now->type == LDS_TOK_IDENT)
    {
        root->tok = now;
        now = now->next;

        if (now->type == LDS_TOK_LEFT_BRACKET)
            root->rch = parse_array_specifier(&now, now);
    }

    *stream = now;
    return root;
}

/// stmt = SEMICOLON |
///        compound_stmt |
///        if_stmt |
///        for_stmt |
///        while_stmt |
///        switch_stmt |
///        CONTINUE SEMICOLON |
///        DISCARD SEMICOLON |
///        RETURN expr? SEMICOLON |
///        BREAK SEMICOLON |
///        decl |
///        expr_stmt
LDShaderNode* LDShaderParserObj::parse_stmt(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderToken* old = now;
    LDShaderNode* root;

    if (now->type == LDS_TOK_SEMICOLON)
    {
        root = mAST->alloc_node(LDS_NODE_EMPTY_STMT);
        *stream = now->next;
        return root;
    }

    if (now->type == LDS_TOK_LEFT_BRACE)
    {
        root = parse_compound_stmt(&now, now);
        *stream = now;
        return root;
    }

    if (now->type == LDS_TOK_IF)
    {
        root = parse_if_stmt(&now, now);
        *stream = now;
        return root;
    }

    if (now->type == LDS_TOK_FOR)
    {
        root = parse_for_stmt(&now, now);
        *stream = now;
        return root;
    }

    if (now->type == LDS_TOK_WHILE)
    {
        root = parse_while_stmt(&now, now);
        *stream = now;
        return root;
    }

    if (now->type == LDS_TOK_SWITCH)
    {
        root = parse_switch_stmt(&now, now);
        *stream = now;
        return root;
    }

    if (now->type == LDS_TOK_CONTINUE || now->type == LDS_TOK_DISCARD || now->type == LDS_TOK_RETURN || now->type == LDS_TOK_BREAK)
    {
        bool isReturn = now->type == LDS_TOK_RETURN;
        root = mAST->alloc_node(LDS_NODE_CONTROL_FLOW_STMT);
        root->tok = now;
        now = now->next;

        if (isReturn && now->type != LDS_TOK_SEMICOLON)
            root->lch = parse_expr(&now, now); // return expression

        if (!consume(&now, LDS_TOK_SEMICOLON))
            return nullptr; // TODO: error

        *stream = now;
        return root;
    }

    if ((root = parse_decl(&now, now)))
    {
        *stream = now;
        return root;
    }

    now = old;
    root = parse_expr_stmt(&now, now);
    *stream = now;
    return root;
}

/// compound_stmt = LEFT_BRACE statement* RIGHT_BRACE
LDShaderNode* LDShaderParserObj::parse_compound_stmt(LDShaderToken** stream, LDShaderToken* now)
{
    if (!consume(&now, LDS_TOK_LEFT_BRACE))
        return nullptr;

    LDShaderNode* root = mAST->alloc_node(LDS_NODE_COMPOUND_STMT);
    LDShaderNode dummy = {.next = nullptr};
    LDShaderNode* stmt = &dummy;

    while (!consume(&now, LDS_TOK_RIGHT_BRACE))
    {
        stmt = stmt->next = parse_stmt(&now, now);

        // TODO: error propagation at statement level
        if (!stmt)
            return nullptr;
    }

    // store statement linked list as left child of compound statement
    root->lch = dummy.next;

    *stream = now;
    return root;
}

/// if_stmt = IF LEFT_PAREN expr RIGHT_PAREN stmt (ELSE (stmt | if_stmt))?
/// @note comparable to selection_statement in the GLSL spec
LDShaderNode* LDShaderParserObj::parse_if_stmt(LDShaderToken** stream, LDShaderToken* now)
{
    if (!consume(&now, LDS_TOK_IF))
        return nullptr;

    if (!consume(&now, LDS_TOK_LEFT_PAREN))
    {
        // TODO: error
        return nullptr;
    }

    LDShaderToken* old = now;
    LDShaderNode* expr = parse_expr(&now, now);

    if (!expr || !consume(&now, LDS_TOK_RIGHT_PAREN))
    {
        // TODO: error
        *stream = old;
        return nullptr;
    }

    LDShaderNode* root = mAST->alloc_node(LDS_NODE_IF_STMT);
    root->cond = expr;
    root->lch = parse_stmt(&now, now);

    if (consume(&now, LDS_TOK_ELSE))
    {
        if (now->type == LDS_TOK_IF)
            root->rch = parse_if_stmt(&now, now);
        else
            root->rch = parse_stmt(&now, now);
    }

    *stream = now;
    return root;
}

/// for_stmt = FOR LEFT_PAREN (decl | expr_stmt | SEMICOLON) expr? SEMICOLON expr? RIGHT_PAREN stmt
LDShaderNode* LDShaderParserObj::parse_for_stmt(LDShaderToken** stream, LDShaderToken* now)
{
    if (!consume(&now, LDS_TOK_FOR))
        return nullptr;

    if (!consume(&now, LDS_TOK_LEFT_PAREN))
    {
        // TODO: error
        return nullptr;
    }

    LDShaderToken* old = now;
    LDShaderNode* init = nullptr;
    LDShaderNode* cond = nullptr;
    LDShaderNode* body = nullptr;
    LDShaderNode* inc = nullptr;

    // for loop init
    if (consume(&now, LDS_TOK_SEMICOLON))
        ;
    else if ((init = parse_decl(&now, now)))
        ;
    else
        init = parse_expr_stmt(&now, now);

    // for loop condition
    if (now->type != LDS_TOK_SEMICOLON)
        cond = parse_expr(&now, now);

    if (!consume(&now, LDS_TOK_SEMICOLON))
    {
        *stream = old; // TODO: error missing ; after condition
        return nullptr;
    }

    // for loop increment
    if (now->type != LDS_TOK_RIGHT_PAREN)
        inc = parse_expr(&now, now);

    if (!consume(&now, LDS_TOK_RIGHT_PAREN))
    {
        *stream = old; // TODO: error missing ) after increment
        return nullptr;
    }

    body = parse_stmt(&now, now);

    LDShaderNode* root = mAST->alloc_node(LDS_NODE_FOR_STMT);
    root->init = init;
    root->cond = cond;
    root->lch = body;
    root->rch = inc;

    *stream = now;
    return root;
}

/// while_stmt = WHILE LEFT_PAREN expr RIGHT_PAREN stmt
LDShaderNode* LDShaderParserObj::parse_while_stmt(LDShaderToken** stream, LDShaderToken* now)
{
    if (!consume(&now, LDS_TOK_WHILE))
        return nullptr;

    if (!consume(&now, LDS_TOK_LEFT_PAREN))
    {
        // TODO: error
        return nullptr;
    }

    LDShaderToken* old = now;
    LDShaderNode* expr = parse_expr(&now, now);

    if (!expr || !consume(&now, LDS_TOK_RIGHT_PAREN))
    {
        // TODO: error
        *stream = old;
        return nullptr;
    }

    LDShaderNode* root = mAST->alloc_node(LDS_NODE_WHILE_STMT);
    root->cond = expr;
    root->lch = parse_stmt(&now, now);

    *stream = now;
    return root;
}

/// switch_stmt = SWITCH LEFT_PAREN expr RIGHT_PAREN LEFT_BRACE switch_case* RIGHT_BRACE
LDShaderNode* LDShaderParserObj::parse_switch_stmt(LDShaderToken** stream, LDShaderToken* now)
{
    if (!consume(&now, LDS_TOK_SWITCH))
        return nullptr;

    if (now->type != LDS_TOK_LEFT_PAREN)
        return nullptr;

    now = now->next;
    LDShaderNode* expr = parse_expr(&now, now);

    if (!consume(&now, LDS_TOK_RIGHT_PAREN) || !consume(&now, LDS_TOK_LEFT_BRACE))
    {
        // TODO: error
        return nullptr;
    }

    LDShaderNode dummy = {.next = nullptr};
    LDShaderNode* kase = &dummy;

    while (!consume(&now, LDS_TOK_RIGHT_BRACE))
    {
        kase = kase->next = parse_switch_case(&now, now);

        if (!kase)
            return nullptr;
    }

    LDShaderNode* root = mAST->alloc_node(LDS_NODE_SWITCH_STMT);
    root->lch = expr;
    root->rch = dummy.next;

    *stream = now;
    return root;
}

/// switch_case = ((CASE expr COLON) | (DEFAULT COLON)) stmt*
LDShaderNode* LDShaderParserObj::parse_switch_case(LDShaderToken** stream, LDShaderToken* now)
{
    if (now->type != LDS_TOK_CASE && now->type != LDS_TOK_DEFAULT)
        return nullptr;

    bool isCase = now->type == LDS_TOK_CASE;
    LDShaderNode* root = mAST->alloc_node(LDS_NODE_SWITCH_CASE);
    root->tok = now;
    now = now->next;

    // case expression is stored as right child
    if (isCase)
        root->rch = parse_expr(&now, now);

    if (!consume(&now, LDS_TOK_COLON))
    {
        // TODO: error missing ':'
        return nullptr;
    }

    LDShaderNode dummy = {.next = nullptr};
    LDShaderNode* stmt = &dummy;

    while (now->type != LDS_TOK_RIGHT_BRACE && now->type != LDS_TOK_CASE && now->type != LDS_TOK_DEFAULT)
    {
        stmt = stmt->next = parse_stmt(&now, now);

        if (!stmt)
            return nullptr;
    }

    // statement linked list stored as left child
    root->lch = dummy.next;

    *stream = now;
    return root;
}

/// expr_stmt = expr SEMICOLON
LDShaderNode* LDShaderParserObj::parse_expr_stmt(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderToken* old = now;
    LDShaderNode* expr = parse_expr(&now, now);

    if (!expr || !consume(&now, LDS_TOK_SEMICOLON))
    {
        *stream = old;
        return nullptr;
    }

    LDShaderNode* root = mAST->alloc_node_lch(LDS_NODE_EXPR_STMT, expr);

    *stream = now;
    return root;
}

/// full_type = (type_qualifier)? type_specifier
/// @note comparable to fully_specified_type in the GLSL spec
LDShaderNode* LDShaderParserObj::parse_full_type(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderToken* old = now;
    LDShaderNode* root = parse_type_specifier(&now, now);

    if (root)
    {
        *stream = now;
        return root;
    }

    LDShaderNode* qualifier = parse_type_qualifier(&now, now);

    if (!qualifier)
    {
        *stream = old;
        return nullptr;
    }

    root = parse_type_specifier(&now, now);

    if (!root)
    {
        *stream = old;
        return nullptr;
    }

    root->lch = qualifier;

    *stream = now;
    return root;
}

/// type_qualifier = single_type_qualifier*
LDShaderNode* LDShaderParserObj::parse_type_qualifier(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root = mAST->alloc_node(LDS_NODE_TYPE_QUALIFIER);
    LDShaderNode dummy = {.next = nullptr};
    LDShaderNode* singleTQ = &dummy;

    while (true)
    {
        singleTQ = singleTQ->next = parse_single_type_qualifier(&now, now);

        if (!singleTQ)
            break;
    }

    if (!dummy.next)
    {
        *stream = now;
        return nullptr;
    }

    root->lch = dummy.next;

    *stream = now;
    return root;
}

/// type_specifier = TYPE_SPECIFIER_TOK (array_specifier)? |
///                  struct_specifier |
///                  STRUCT_IDENT
LDShaderNode* LDShaderParserObj::parse_type_specifier(LDShaderToken** stream, LDShaderToken* now)
{
    if (now->type == LDS_TOK_STRUCT)
        return parse_struct_specifier(stream, now);

    if (!is_type_specifier_tok(now) && !is_struct_ident(now))
        return nullptr;

    LDShaderNode* root = mAST->alloc_node(LDS_NODE_TYPE_SPECIFIER);
    root->tok = now;
    now = now->next;

    if (now->type == LDS_TOK_LEFT_BRACKET)
        root->lch = parse_array_specifier(&now, now);

    *stream = now;
    return root;
}

/// struct_specifier = struct IDENT? struct_decl
LDShaderNode* LDShaderParserObj::parse_struct_specifier(LDShaderToken** stream, LDShaderToken* now)
{
    if (!consume(&now, LDS_TOK_STRUCT))
        return nullptr;

    LDShaderNode* root = mAST->alloc_node(LDS_NODE_STRUCT_SPECIFIER);

    if (now->type == LDS_TOK_IDENT)
    {
        root->tok = now; // struct name

        // register struct identifier
        Hash32 hash = hash32_FNV_1a(now->pos, now->len);
        mStructIdent.insert(hash);

        now = now->next;
    }

    if (now->type != LDS_TOK_LEFT_BRACE)
        return nullptr;

    root->lch = parse_struct_decl(&now, now);

    *stream = now;
    return root;
}

/// struct_decl = LEFT_BRACE (struct_member)* RIGHT_BRACE (IDENT array_specifier?)?
LDShaderNode* LDShaderParserObj::parse_struct_decl(LDShaderToken** stream, LDShaderToken* now)
{
    if (!consume(&now, LDS_TOK_LEFT_BRACE))
        return nullptr;

    LDShaderNode* root = mAST->alloc_node(LDS_NODE_STRUCT_DECL);
    LDShaderNode dummy = {.next = nullptr};
    LDShaderNode* member = &dummy;

    while (!consume(&now, LDS_TOK_RIGHT_BRACE))
    {
        member = member->next = parse_struct_member(&now, now);
    }

    // store array member linked list as left child
    root->lch = dummy.next;

    if (now->type == LDS_TOK_IDENT)
    {
        root->tok = now;
        now = now->next;

        if (now->type == LDS_TOK_LEFT_BRACKET)
            root->rch = parse_array_specifier(&now, now);
    }

    *stream = now;
    return root;
}

/// struct_member = (full_type | STRUCT_IDENT) IDENT array_specifier? SEMICOLON
/// @note we prohibit comma separated identifiers during a single member declaration
LDShaderNode* LDShaderParserObj::parse_struct_member(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* fullType = parse_full_type(&now, now);

    if (!fullType)
    {
        // TODO: error unknown member type
        return nullptr;
    }

    if (now->type != LDS_TOK_IDENT)
    {
        // TODO: error missing member name
        return nullptr;
    }

    LDShaderNode* root = mAST->alloc_node_lch(LDS_NODE_STRUCT_MEMBER, fullType);
    root->tok = now; // member name
    now = now->next;

    if (now->type == LDS_TOK_COMMA)
    {
        // TODO: error
        return nullptr;
    }

    if (now->type == LDS_TOK_LEFT_BRACKET)
        root->rch = parse_array_specifier(&now, now);

    if (!consume(&now, LDS_TOK_SEMICOLON))
    {
        // TODO: error
        return nullptr;
    }

    *stream = now;
    return root;
}

/// array_specifier = (LEFT_BRACKET (conditional)? RIGHT_BRACKET)*
LDShaderNode* LDShaderParserObj::parse_array_specifier(LDShaderToken** stream, LDShaderToken* now)
{
    if (now->type != LDS_TOK_LEFT_BRACKET)
        return nullptr;

    LDShaderNode dummy = {.next = nullptr};
    LDShaderNode* arr = &dummy;

    while (consume(&now, LDS_TOK_LEFT_BRACKET))
    {
        arr = arr->next = mAST->alloc_node(LDS_NODE_ARRAY_SPECIFIER);

        if (now->type != LDS_TOK_RIGHT_BRACKET)
        {
            LDShaderNode* arrSize = parse_conditional(&now, now);

            if (!arrSize)
            {
                // TODO: error unrecognized array size
                return nullptr;
            }

            arr->lch = arrSize;
        }

        if (!consume(&now, LDS_TOK_RIGHT_BRACKET))
        {
            // TODO: error expected right bracket
            return nullptr;
        }
    }

    *stream = now;
    return dummy.next;
}

/// single_type_qualifier = layout_qualifier |
///                         storage_qualifier
LDShaderNode* LDShaderParserObj::parse_single_type_qualifier(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root;

    if ((root = parse_storage_qualifer(&now, now)) ||
        (root = parse_layout_qualifier(&now, now)))
    {
        *stream = now;
        return root;
    }

    return nullptr;
}

LDShaderNode* LDShaderParserObj::parse_storage_qualifer(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root = nullptr;

    if (is_storage_qualifier_tok(now))
    {
        root = mAST->alloc_node(LDS_NODE_STORAGE_QUALIFIER);
        root->tok = now;
        now = now->next;
    }

    *stream = now;
    return root;
}

/// layout_qualifier = LAYOUT LEFT_PAREN layout_qualifier_id (COMMA layout_qualifier_id)* RIGHT_PAREN
LDShaderNode* LDShaderParserObj::parse_layout_qualifier(LDShaderToken** stream, LDShaderToken* now)
{
    if (!consume(&now, LDS_TOK_LAYOUT))
        return nullptr;

    if (!consume(&now, LDS_TOK_LEFT_PAREN))
    {
        // TODO: error
        return nullptr;
    }

    LDShaderNode* root = mAST->alloc_node(LDS_NODE_LAYOUT_QUALIFIER);
    LDShaderNode* qualifierID = root->lch = parse_layout_qualifier_id(&now, now);

    if (!qualifierID)
    {
        // TODO: error
        return nullptr;
    }

    while (now->type != LDS_TOK_RIGHT_PAREN)
    {
        if (!consume(&now, LDS_TOK_COMMA))
        {
            // TODO: error
            return nullptr;
        }

        qualifierID = qualifierID->next = parse_layout_qualifier_id(&now, now);
    }

    consume(&now, LDS_TOK_RIGHT_PAREN);

    *stream = now;
    return root;
}

/// layout_qualifier_id = IDENT (EQUAL conditional)?
LDShaderNode* LDShaderParserObj::parse_layout_qualifier_id(LDShaderToken** stream, LDShaderToken* now)
{
    if (now->type != LDS_TOK_IDENT)
        return nullptr;

    LDShaderNode* root = mAST->alloc_node(LDS_NODE_LAYOUT_QUALIFIER_ID);
    root->tok = now;
    now = now->next;

    if (consume(&now, LDS_TOK_EQUAL))
    {
        root->lch = parse_conditional(&now, now);
    }

    *stream = now;
    return root;
}

/// initializer = LEFT_BRACE initializer_list COMMA? RIGHT_BRACE |
///               assignment
LDShaderNode* LDShaderParserObj::parse_initializer(LDShaderToken** stream, LDShaderToken* now)
{
    if (!consume(&now, LDS_TOK_LEFT_BRACE))
        return parse_assignment(stream, now);

    LDShaderToken* old = now;
    LDShaderNode* root = mAST->alloc_node(LDS_NODE_INITIALIZER);
    root->lch = parse_initializer_list(&now, now);

    consume(&now, LDS_TOK_COMMA);

    if (consume(&now, LDS_TOK_RIGHT_BRACE))
    {
        *stream = now;
        return root;
    }

    // NOTE: by default GLSL does not allow null initializers,
    //       see the discussion in GL_EXT_null_initializer,
    //       we do not support the extension here either.
    *stream = old;
    return nullptr;
}

/// initializer_list = initializer (COMMA initializer)*
LDShaderNode* LDShaderParserObj::parse_initializer_list(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root = parse_initializer(&now, now);
    LDShaderNode* last = root;

    if (!root)
    {
        // TODO: error null initializer
        return nullptr;
    }

    while (now->type == LDS_TOK_COMMA)
    {
        LDShaderToken* next = now->next;

        if (next->type == LDS_TOK_RIGHT_BRACE)
            break;

        now = next;
        last = last->next = parse_initializer(&now, now);
    }

    *stream = now;
    return root;
}

/// expr = assignment (COMMA assignment)*
LDShaderNode* LDShaderParserObj::parse_expr(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root = parse_assignment(&now, now);
    LDShaderNode* last = root;

    while (consume(&now, LDS_TOK_COMMA))
    {
        last = last->next = parse_assignment(&now, now);
    }

    *stream = now;
    return root;
}

/// assignment = conditional (ASSIGNMENT_TOK assignment)?
/// @note The conditional rule in the GLSL spec is the ternary operator
///       and can only be a rvalue, the assignment grammar allows the
///       conditional to be a lvalue only for ease of parsing.
LDShaderNode* LDShaderParserObj::parse_assignment(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root = parse_conditional(&now, now);

    if (is_assignment_tok(now))
    {
        root = mAST->alloc_node_lch(LDS_NODE_ASSIGNMENT, root);
        root->tok = now; // assignment operator token
        now = now->next;

        // assignment lvalue as left child, rvalue as right child
        root->rch = parse_assignment(&now, now);
    }

    *stream = now;
    return root;
}

/// conditional = logical_or (QUESTION expr COLON assignment)?
LDShaderNode* LDShaderParserObj::parse_conditional(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root = parse_logical_or(&now, now);

    if (consume(&now, LDS_TOK_QUESTION))
    {
        LDShaderNode* lch = parse_expr(&now, now);

        if (!consume(&now, LDS_TOK_COLON))
        {
            return nullptr;
        }

        LDShaderNode* rch = parse_assignment(&now, now);
        LDShaderNode* cond = mAST->alloc_node(LDS_NODE_CONDITIONAL);
        cond->lch = lch;
        cond->rch = rch;
        root = cond;
    }

    *stream = now;
    return root;
}

/// logical_or = logical_xor (OR_OP logical_xor)*
LDShaderNode* LDShaderParserObj::parse_logical_or(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root = parse_logical_xor(&now, now);

    while (now->type == LDS_TOK_OR_OP)
    {
        root = mAST->alloc_node_lch(LDS_NODE_LOGICAL_OR, root);
        root->tok = now;
        now = now->next;
        root->rch = parse_logical_xor(&now, now);
    }

    *stream = now;
    return root;
}

/// logical_xor = logical_and (XOR_OP logical_and)*
LDShaderNode* LDShaderParserObj::parse_logical_xor(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root = parse_logical_and(&now, now);

    while (now->type == LDS_TOK_XOR_OP)
    {
        root = mAST->alloc_node_lch(LDS_NODE_LOGICAL_XOR, root);
        root->tok = now;
        now = now->next;
        root->rch = parse_logical_and(&now, now);
    }

    *stream = now;
    return root;
}

/// logical_and = bitwise_or (AND_OP bitwise_or)*
LDShaderNode* LDShaderParserObj::parse_logical_and(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root = parse_bitwise_or(&now, now);

    while (now->type == LDS_TOK_AND_OP)
    {
        root = mAST->alloc_node_lch(LDS_NODE_LOGICAL_AND, root);
        root->tok = now;
        now = now->next;
        root->rch = parse_bitwise_or(&now, now);
    }

    *stream = now;
    return root;
}

/// bitwise_or = bitwise_xor (VERTICAL_BAR bitwise_xor)*
LDShaderNode* LDShaderParserObj::parse_bitwise_or(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root = parse_bitwise_xor(&now, now);

    while (now->type == LDS_TOK_VERTICAL_BAR)
    {
        root = mAST->alloc_node_lch(LDS_NODE_BITWISE_OR, root);
        root->tok = now;
        now = now->next;
        root->rch = parse_bitwise_xor(&now, now);
    }

    *stream = now;
    return root;
}

/// bitwise_xor = bitwise_and (CARET bitwise_and)*
LDShaderNode* LDShaderParserObj::parse_bitwise_xor(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root = parse_bitwise_and(&now, now);

    while (now->type == LDS_TOK_CARET)
    {
        root = mAST->alloc_node_lch(LDS_NODE_BITWISE_XOR, root);
        root->tok = now;
        now = now->next;
        root->rch = parse_bitwise_and(&now, now);
    }

    *stream = now;
    return root;
}

/// bitwise_and = equal (AMPERSAND equal)*
LDShaderNode* LDShaderParserObj::parse_bitwise_and(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root = parse_equal(&now, now);

    while (now->type == LDS_TOK_AMPERSAND)
    {
        root = mAST->alloc_node_lch(LDS_NODE_BITWISE_AND, root);
        root->tok = now;
        now = now->next;
        root->rch = parse_equal(&now, now);
    }

    *stream = now;
    return root;
}

/// equal = relational ((EQ_OP | NE_OP) relational)*
LDShaderNode* LDShaderParserObj::parse_equal(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root = parse_relational(&now, now);

    while (now->type == LDS_TOK_EQ_OP || now->type == LDS_TOK_NE_OP)
    {
        root = mAST->alloc_node_lch(LDS_NODE_EQUAL, root);
        root->tok = now;
        now = now->next;
        root->rch = parse_relational(&now, now);
    }

    *stream = now;
    return root;
}

/// relational = shift ((LEFT_ANGLE | RIGHT_ANGLE | LE_OP | GE_OP) shift)*
LDShaderNode* LDShaderParserObj::parse_relational(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root = parse_shift(&now, now);

    while (now->type == LDS_TOK_LEFT_ANGLE || now->type == LDS_TOK_RIGHT_ANGLE || now->type == LDS_TOK_LE_OP || now->type == LDS_TOK_GE_OP)
    {
        root = mAST->alloc_node_lch(LDS_NODE_RELATIONAL, root);
        root->tok = now;
        now = now->next;
        root->rch = parse_relational(&now, now);
    }

    *stream = now;
    return root;
}

/// shift = add ((LEFT_OP | RIGHT_OP) add)*
LDShaderNode* LDShaderParserObj::parse_shift(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root = parse_add(&now, now);

    while (now->type == LDS_TOK_LEFT_OP || now->type == LDS_TOK_RIGHT_OP)
    {
        root = mAST->alloc_node_lch(LDS_NODE_SHIFT, root);
        root->tok = now;
        now = now->next;
        root->rch = parse_add(&now, now);
    }

    *stream = now;
    return root;
}

/// add = mul ((PLUS | DASH) mul)*
LDShaderNode* LDShaderParserObj::parse_add(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root = parse_mul(&now, now);

    while (now->type == LDS_TOK_PLUS || now->type == LDS_TOK_DASH)
    {
        root = mAST->alloc_node_lch(LDS_NODE_ADD, root);
        root->tok = now;
        now = now->next;
        root->rch = parse_mul(&now, now);
    }

    *stream = now;
    return root;
}

/// mul = unary ((STAR | SLASH | PERCENT) unary)*
LDShaderNode* LDShaderParserObj::parse_mul(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root = parse_unary(&now, now);

    while (now->type == LDS_TOK_STAR || now->type == LDS_TOK_SLASH || now->type == LDS_TOK_PERCENT)
    {
        root = mAST->alloc_node_lch(LDS_NODE_MUL, root);
        root->tok = now;
        now = now->next;
        root->rch = parse_unary(&now, now);
    }

    *stream = now;
    return root;
}

/// unary = UNARY_TOK unary |
///         postfix
LDShaderNode* LDShaderParserObj::parse_unary(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root;

    if (is_unary_tok(now))
    {
        LDShaderToken* unaryTok = now;
        now = now->next;
        root = mAST->alloc_node_lch(LDS_NODE_UNARY, parse_unary(&now, now));
        root->tok = unaryTok;

        *stream = now;
        return root;
    }

    root = parse_postfix(&now, now);

    *stream = now;
    return root;
}

/// postfix = primary (postfix_expr)*
LDShaderNode* LDShaderParserObj::parse_postfix(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root = parse_primary(&now, now);

    while (is_postfix_tok(now))
    {
        LDShaderNode* postfix = parse_postfix_expr(&now, now);
        LD_ASSERT(postfix);

        postfix->lch = root;
        root = postfix;
    }

    *stream = now;
    return root;
}

/// postfix_expr = LEFT_BRACKET expr RIGHT_BRACKET |
///                INC_OP |
///                DEC_OP |
///                call
LDShaderNode* LDShaderParserObj::parse_postfix_expr(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root;
    LDShaderToken* old = now;

    // index postfix expression
    if (consume(&now, LDS_TOK_LEFT_BRACKET))
    {
        root = mAST->alloc_node(LDS_NODE_INDEX);
        root->rch = parse_expr(&now, now);

        if (!consume(&now, LDS_TOK_RIGHT_BRACKET))
            return nullptr; // TODO: error

        *stream = now;
        return root;
    }

    if ((now->type == LDS_TOK_INC_OP) || (now->type == LDS_TOK_DEC_OP))
    {
        root = mAST->alloc_node(LDS_NODE_POSTFIX);
        root->tok = now;
        *stream = now->next;
        return root;
    }

    if (now->type == LDS_TOK_LEFT_PAREN)
    {
        root = parse_call(&now, now);
        *stream = now;
        return root;
    }

    *stream = old;
    return nullptr;
}

/// call = LEFT_PAREN (assignment (COMMA assignment)*)? RIGHT_PAREN
/// @note the GLSL constructor syntax allows type names to be called upon. vec4(), mat3(), etc.
LDShaderNode* LDShaderParserObj::parse_call(LDShaderToken** stream, LDShaderToken* now)
{
    if (!consume(&now, LDS_TOK_LEFT_PAREN))
        return nullptr;

    LDShaderNode* root = mAST->alloc_node(LDS_NODE_CALL);
    LDShaderNode dummy = {.next = nullptr};
    LDShaderNode* arg = &dummy;

    int argc = 0;

    while (!consume(&now, LDS_TOK_RIGHT_PAREN))
    {
        if (argc > 0 && !consume(&now, LDS_TOK_COMMA))
        {
            // TODO: error missing comma
            return nullptr;
        }

        arg = arg->next = parse_assignment(&now, now);
        LD_ASSERT(arg);

        argc++;
    }

    root->rch = dummy.next;
    *stream = now;
    return root;
}

/// primary = ident |
///           CONSTANT |
///           type_specifier
LDShaderNode* LDShaderParserObj::parse_primary(LDShaderToken** stream, LDShaderToken* now)
{
    LDShaderNode* root;

    if (now->type == LDS_TOK_IDENT)
    {
        root = mAST->alloc_node(LDS_NODE_VAR);
        root->tok = now;
        *stream = now->next;
        return root;
    }

    if (now->type == LDS_TOK_INT_CONSTANT || now->type == LDS_TOK_UINT_CONSTANT || now->type == LDS_TOK_BOOL_CONSTANT)
    {
        root = mAST->alloc_node(LDS_NODE_CONSTANT);
        root->tok = now;
        *stream = now->next;
        return root;
    }

    if ((root = parse_type_specifier(&now, now)))
    {
        *stream = now;
        return root;
    }

    LD_UNREACHABLE;
    return nullptr;
}

LDShaderAST LDShaderParserObj::parse(const char* str, size_t strLen, LDShaderType type)
{
    // make a copy of source string, Tokens are string views into source code
    mSource = std::string(str, strLen);

    // TODO: keep track of all ASTs parsed by this parser
    mAST = heap_new<LDShaderASTObj>(MEMORY_USAGE_MISC);

    // prepares Token stream in mTokens field
    tokenize(str, strLen);

    LDShaderToken* stream = mTokens;
    mAST->root = parse_translation_unit(&stream, stream);

    return {mAST};
}

bool LDShaderAST::is_valid()
{
    return mObj->root != nullptr;
}

void LDShaderAST::traverse(TraverseFn fn, void* user)
{
    if (!mObj->root)
        return;

    int depth = 0;
    recursive_traverse(mObj->root, fn, depth, user);
}

LDShaderNode* LDShaderAST::get_root()
{
    return mObj->root;
}

std::string LDShaderAST::print()
{
    std::string str;

    traverse(&print_node_fn, &str);

    return str;
}

const char* LDShaderAST::get_node_type_cstr(LDShaderNodeType type)
{
    return sNodeTable[(int)type].cstr;
}

LDShaderParser LDShaderParser::create()
{
    LDShaderParserObj* obj = heap_new<LDShaderParserObj>(MEMORY_USAGE_MISC);

    return {obj};
}

void LDShaderParser::destroy(LDShaderParser parser)
{
    LDShaderParserObj* obj = parser;

    heap_delete<LDShaderParserObj>(obj);
}

LDShaderAST LDShaderParser::parse(const char* ldshader, size_t len, LDShaderType type)
{
    LD_PROFILE_SCOPE;

    return mObj->parse(ldshader, len, type);
}

} // namespace LD
