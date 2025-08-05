#include "Extra/doctest/doctest.h"
#include <Ludens/RenderBackend/LDShaderCompiler.h>
#include <Ludens/RenderBackend/LDShaderParser.h>
#include <cstring>
#include <iostream>
#include <vector>

using namespace LD;

struct ASTNode
{
    LDShaderNodeType type;
    const char* tokenCstr;
};

static inline bool tok_equals(const LDShaderToken* tok, const char* cstr)
{
    size_t len = strlen(cstr);
    if (tok->len != len)
        return false;

    return !strncmp(tok->pos, cstr, len);
}

struct ASTValidator
{
    const ASTNode* expected;
    size_t expectedCount;
    size_t nodeCounter;
    bool hasFailed;

    static void validateNode(const LDShaderNode* root, int depth, void* user)
    {
        ASTValidator& self = *(ASTValidator*)user;

        if (self.hasFailed)
            return;

        if (self.nodeCounter == self.expectedCount)
        {
            self.hasFailed = true;
            printf("found more nodes in AST than expected: %d\n", (int)self.expectedCount);
            return;
        }

        const ASTNode* expectedRoot = self.expected + self.nodeCounter++;

        bool tokenMismatch = (root->tok && !expectedRoot->tokenCstr) || (!root->tok && expectedRoot->tokenCstr);
        if (root->tok && expectedRoot->tokenCstr)
        {
            tokenMismatch = !tok_equals(root->tok, expectedRoot->tokenCstr);
        }

        if (root->type != expectedRoot->type || tokenMismatch)
        {
            self.hasFailed = true;
            const char* foundNodeCstr = LDShaderAST::get_node_type_cstr(root->type);
            const char* expectedNodeCstr = LDShaderAST::get_node_type_cstr(expectedRoot->type);
            size_t expectedTokenLen = expectedRoot->tokenCstr ? strlen(expectedRoot->tokenCstr) : 0;
            size_t foundTokenLen = root->tok ? root->tok->len : 0;
            const char* foundTokenStr = root->tok ? root->tok->pos : nullptr;
            printf("expected [%s:%.*s], found [%s:%.*s].\n",
                   expectedNodeCstr,
                   expectedTokenLen, expectedRoot->tokenCstr,
                   foundNodeCstr,
                   foundTokenLen, foundTokenStr);
        }
    }
};

TEST_CASE("LDShaderParser type qualifiers")
{
    LDShaderParser parser = LDShaderParser::create();

    // regression testing for some type qualifiers
    static const char glsl[] = R"(
layout (location = 0) in vec3 aPos;
layout (local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
layout (set = 0, binding = 0, rgba8ui) readonly uniform uimage2D sImage;
)";
    constexpr size_t glslLen = sizeof(glsl) - 1;

    // clang-format off
    std::vector<ASTNode> expectedAST = {
        { LDS_NODE_TRANSLATION_UNIT, nullptr },
        {   LDS_NODE_SINGLE_DECL, "aPos" },
        {     LDS_NODE_TYPE_SPECIFIER, "vec3" },
        {       LDS_NODE_TYPE_QUALIFIER, nullptr },
        {         LDS_NODE_LAYOUT_QUALIFIER, nullptr },
        {           LDS_NODE_LAYOUT_QUALIFIER_ID, "location" },
        {             LDS_NODE_CONSTANT, "0" },
        {         LDS_NODE_STORAGE_QUALIFIER, "in" },
        {   LDS_NODE_SINGLE_DECL, nullptr },
        {     LDS_NODE_TYPE_QUALIFIER, nullptr },
        {       LDS_NODE_LAYOUT_QUALIFIER, nullptr },
        {         LDS_NODE_LAYOUT_QUALIFIER_ID, "local_size_x" },
        {           LDS_NODE_CONSTANT, "8" },
        {         LDS_NODE_LAYOUT_QUALIFIER_ID, "local_size_y" },
        {           LDS_NODE_CONSTANT, "8" },
        {         LDS_NODE_LAYOUT_QUALIFIER_ID, "local_size_z" },
        {           LDS_NODE_CONSTANT, "1" },
        {       LDS_NODE_STORAGE_QUALIFIER, "in" },
        {   LDS_NODE_SINGLE_DECL, "sImage" },
        {     LDS_NODE_TYPE_SPECIFIER, "uimage2D" },
        {       LDS_NODE_TYPE_QUALIFIER, nullptr },
        {         LDS_NODE_LAYOUT_QUALIFIER, nullptr },
        {           LDS_NODE_LAYOUT_QUALIFIER_ID, "set" },
        {             LDS_NODE_CONSTANT, "0" },
        {           LDS_NODE_LAYOUT_QUALIFIER_ID, "binding" },
        {             LDS_NODE_CONSTANT, "0" },
        {           LDS_NODE_LAYOUT_QUALIFIER_ID, "rgba8ui" },
        {         LDS_NODE_STORAGE_QUALIFIER, "readonly" },
        {         LDS_NODE_STORAGE_QUALIFIER, "uniform" },
    };
    // clang-format on

    ASTValidator validator{};
    validator.expected = expectedAST.data();
    validator.expectedCount = expectedAST.size();

    LDShaderAST ast = parser.parse(glsl, glslLen, LD_SHADER_TYPE_VERTEX);
    ast.traverse(ASTValidator::validateNode, &validator);

    // if this fails, use ast.print() to dump the incorrect AST
    CHECK(!validator.hasFailed);

    LDShaderParser::destroy(parser);
}

TEST_CASE("LDShaderParser function prototype")
{
    LDShaderParser parser = LDShaderParser::create();

    static const char glsl[] = R"(
void main();
void foo(void);
out int bar(in mat2 p1, out float p2[4u], inout vec4 p3);
)";
    constexpr size_t glslLen = sizeof(glsl) - 1;

    // clang-format off
    std::vector<ASTNode> expectedAST = {
        { LDS_NODE_TRANSLATION_UNIT, nullptr },
        {   LDS_NODE_FN_PROTOTYPE, "main" },
        {     LDS_NODE_TYPE_SPECIFIER, "void" },
        {   LDS_NODE_FN_PROTOTYPE, "foo" },
        {     LDS_NODE_TYPE_SPECIFIER, "void" },
        {     LDS_NODE_FN_PARAM_DECL, nullptr },
        {       LDS_NODE_TYPE_SPECIFIER, "void" },
        {   LDS_NODE_FN_PROTOTYPE, "bar" },
        {     LDS_NODE_TYPE_SPECIFIER, "int" },
        {       LDS_NODE_TYPE_QUALIFIER, nullptr },
        {         LDS_NODE_STORAGE_QUALIFIER, "out" },
        {     LDS_NODE_FN_PARAM_DECL, "p1" },
        {       LDS_NODE_TYPE_SPECIFIER, "mat2" },
        {         LDS_NODE_TYPE_QUALIFIER, nullptr },
        {           LDS_NODE_STORAGE_QUALIFIER, "in" },
        {     LDS_NODE_FN_PARAM_DECL, "p2" },
        {       LDS_NODE_TYPE_SPECIFIER, "float" },
        {         LDS_NODE_TYPE_QUALIFIER, nullptr },
        {           LDS_NODE_STORAGE_QUALIFIER, "out" },
        {       LDS_NODE_ARRAY_SPECIFIER, nullptr },
        {         LDS_NODE_CONSTANT, "4u" },
        {     LDS_NODE_FN_PARAM_DECL, "p3" },
        {       LDS_NODE_TYPE_SPECIFIER, "vec4" },
        {         LDS_NODE_TYPE_QUALIFIER, nullptr },
        {           LDS_NODE_STORAGE_QUALIFIER, "inout" },
    };
    // clang-format on

    ASTValidator validator{};
    validator.expected = expectedAST.data();
    validator.expectedCount = expectedAST.size();

    LDShaderAST ast = parser.parse(glsl, glslLen, LD_SHADER_TYPE_VERTEX);
    ast.traverse(ASTValidator::validateNode, &validator);

    // if this fails, use ast.print() to dump the incorrect AST
    CHECK(!validator.hasFailed);

    LDShaderParser::destroy(parser);
}

TEST_CASE("LDShaderParser")
{
    LDShaderParser parser = LDShaderParser::create();

    static const char glsl[] = R"(
layout (local_size_x = 8) in;

layout (set = 1, binding = 0, rgba8ui) readonly uniform uimage2D sImage;

struct PickQuery
{
    uvec2 pos;   // picking position
    uint result; // picking result
    uint pad;    // padding for array alignment
};
layout (set = 1, binding = 1, std430) buffer QueryBuffer {
    PickQuery queries[];
} sQueryBuffer;
/*
void main()
{
    uint i = uint(gl_GlobalInvocationID.x);

    uint result = 0;
    uvec4 texel = imageLoad(sImage, ivec2(sQueryBuffer.queries[i].pos));
    result |= (texel.r & 0xFF);
    result |= (texel.g & 0xFF) << 8;
    result |= (texel.b & 0xFF) << 16;
    result |= (texel.a & 0xFF) << 24;
    sQueryBuffer.queries[i].result = result;
}
*/
)";
    constexpr size_t glslLen = sizeof(glsl) - 1;

    LDShaderAST ast = parser.parse(glsl, glslLen, LD_SHADER_TYPE_VERTEX);
    std::string str = ast.print();
    std::cout << str << std::endl;

    LDShaderCompilerVulkan compiler = LDShaderCompilerVulkan::create();

    std::vector<uint32_t> spirv;
    compiler.compile(ast, spirv);

    LDShaderCompilerVulkan::destroy(compiler);
    LDShaderParser::destroy(parser);
}
