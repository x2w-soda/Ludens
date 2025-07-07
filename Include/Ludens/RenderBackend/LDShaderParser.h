#pragma once

#include <Ludens/Header/Handle.h>
#include <cstddef>
#include <string>

namespace LD {

enum LDShaderType
{
    LD_SHADER_TYPE_VERTEX,
    LD_SHADER_TYPE_FRAGMENT,
};

enum LDShaderTokenType
{
    LDS_TOK_EOF = 0,
    LDS_TOK_IDENT,
    LDS_TOK_INT_CONSTANT,
    LDS_TOK_CONST,
    LDS_TOK_STRUCT,
    LDS_TOK_VOID,
    LDS_TOK_FLOAT,
    LDS_TOK_DOUBLE,
    LDS_TOK_INT,
    LDS_TOK_UINT,
    LDS_TOK_BOOL,
    LDS_TOK_VEC2,
    LDS_TOK_VEC3,
    LDS_TOK_VEC4,
    LDS_TOK_DVEC2,
    LDS_TOK_DVEC3,
    LDS_TOK_DVEC4,
    LDS_TOK_BVEC2,
    LDS_TOK_BVEC3,
    LDS_TOK_BVEC4,
    LDS_TOK_IVEC2,
    LDS_TOK_IVEC3,
    LDS_TOK_IVEC4,
    LDS_TOK_UVEC2,
    LDS_TOK_UVEC3,
    LDS_TOK_UVEC4,
    LDS_TOK_MAT2,
    LDS_TOK_MAT3,
    LDS_TOK_MAT4,
    LDS_TOK_DMAT2,
    LDS_TOK_DMAT3,
    LDS_TOK_DMAT4,
    LDS_TOK_SAMPLER1D,
    LDS_TOK_SAMPLER1DARRAY,
    LDS_TOK_SAMPLER1DARRAYSHADOW,
    LDS_TOK_SAMPLER1DSHADOW,
    LDS_TOK_SAMPLER2D,
    LDS_TOK_SAMPLER2DARRAY,
    LDS_TOK_SAMPLER2DARRAYSHADOW,
    LDS_TOK_SAMPLER2DSHADOW,
    LDS_TOK_SAMPLER3D,
    LDS_TOK_SAMPLERCUBE,
    LDS_TOK_SAMPLERCUBEARRAY,
    LDS_TOK_SAMPLERCUBEARRAYSHADOW,
    LDS_TOK_SAMPLERCUBESHADOW,
    LDS_TOK_IN,
    LDS_TOK_OUT,
    LDS_TOK_INOUT,
    LDS_TOK_UNIFORM,
    LDS_TOK_PATCH,
    LDS_TOK_SAMPLE,
    LDS_TOK_BUFFER,
    LDS_TOK_SHARED,
    LDS_TOK_COHERENT,
    LDS_TOK_VOLATILE,
    LDS_TOK_RESTRICT,
    LDS_TOK_READONLY,
    LDS_TOK_WRITEONLY,
    LDS_TOK_NOPERSPECTIVE,
    LDS_TOK_FLAT,
    LDS_TOK_SMOOTH,
    LDS_TOK_LAYOUT,
    LDS_TOK_LEFT_OP,       // <<
    LDS_TOK_RIGHT_OP,      // >>
    LDS_TOK_INC_OP,        // ++
    LDS_TOK_DEC_OP,        // --
    LDS_TOK_LE_OP,         // <=
    LDS_TOK_GE_OP,         // >=
    LDS_TOK_EQ_OP,         // ==
    LDS_TOK_NE_OP,         // !=
    LDS_TOK_AND_OP,        // &&
    LDS_TOK_OR_OP,         // ||
    LDS_TOK_XOR_OP,        // ^^
    LDS_TOK_ADD_ASSIGN,    // +=
    LDS_TOK_SUB_ASSIGN,    // -=
    LDS_TOK_MUL_ASSIGN,    // *=
    LDS_TOK_DIV_ASSIGN,    // /=
    LDS_TOK_MOD_ASSIGN,    // %=
    LDS_TOK_LEFT_ASSIGN,   // <<=
    LDS_TOK_RIGHT_ASSIGN,  // >>=
    LDS_TOK_AND_ASSIGN,    // &=
    LDS_TOK_XOR_ASSIGN,    // ^=
    LDS_TOK_OR_ASSIGN,     // |=
    LDS_TOK_LEFT_PAREN,    // (
    LDS_TOK_RIGHT_PAREN,   // )
    LDS_TOK_LEFT_BRACKET,  // [
    LDS_TOK_RIGHT_BRACKET, // ]
    LDS_TOK_LEFT_BRACE,    // {
    LDS_TOK_RIGHT_BRACE,   // }
    LDS_TOK_DOT,           // .
    LDS_TOK_COMMA,         // ,
    LDS_TOK_COLON,         // :
    LDS_TOK_EQUAL,         // ==
    LDS_TOK_SEMICOLON,     // ;
    LDS_TOK_BANG,          // !
    LDS_TOK_DASH,          // -
    LDS_TOK_TILDE,         // ~
    LDS_TOK_PLUS,          // +
    LDS_TOK_STAR,          // *
    LDS_TOK_SLASH,         // /
    LDS_TOK_PERCENT,       // %
    LDS_TOK_LEFT_ANGLE,    // <
    LDS_TOK_RIGHT_ANGLE,   // >
    LDS_TOK_VERTICAL_BAR,  // |
    LDS_TOK_CARET,         // ^
    LDS_TOK_AMPERSAND,     // &
    LDS_TOK_QUESTION,      // ?
    LDS_TOK_ENUM_COUNT,
};

struct LDShaderToken
{
    LDShaderToken* next;    /// token linked list
    const char* pos;        /// token begin position
    int len;                /// token content length
    int line;               /// line in ldshader source code
    int col;                /// column in ldshader source code
    LDShaderTokenType type; /// token type
};

enum LDShaderNodeType
{
    LDS_NODE_TRANSLATION_UNIT = 0,
    LDS_NODE_SINGLE_DECL,
    LDS_NODE_FN_PROTOTYPE,
    LDS_NODE_FN_DEFINITION,
    LDS_NODE_COMPOUND_STMT,
    LDS_NODE_TYPE_SPECIFIER,
    LDS_NODE_TYPE_QUALIFIER,
    LDS_NODE_LAYOUT_QUALIFIER,
    LDS_NODE_LAYOUT_QUALIFIER_ID,
    LDS_NODE_STORAGE_QUALIFIER,
    LDS_NODE_ASSIGNMENT,
    LDS_NODE_CONDITIONAL,
    LDS_NODE_ADD,
    LDS_NODE_MUL,
    LDS_NODE_VAR,
    LDS_NODE_CONSTANT,
    LDS_NODE_ENUM_COUNT
};

struct LDShaderNode
{
    LDShaderNode* next;    /// sibling or linked list
    LDShaderNode* lch;     /// left child root
    LDShaderNode* rch;     /// right child root
    LDShaderToken* tok;    /// representative token of the node
    LDShaderNodeType type; /// root type
};

/// @brief The ldshader abstract syntax tree representation.
struct LDShaderAST : Handle<struct LDShaderASTObj>
{
    typedef void (*TraverseFn)(const LDShaderNode* root, int depth, void* user);

    bool is_valid();

    /// @brief pre-order tree traversal with user callback function
    /// @param fn user callback function to be called on each node
    void traverse(TraverseFn fn, void* user);

    /// @brief debug print the AST
    std::string print();
};

/// @brief The ldshader frontend parser. Frontend is responsible for preprocessing source code,
///        resolving compile-constants, and provide error diagnostics.
struct LDShaderParser : Handle<struct LDShaderParserObj>
{
    static LDShaderParser create();
    static void destroy(LDShaderParser parser);

    LDShaderAST parse(const char* ldshader, size_t len, LDShaderType type);
};

} // namespace LD