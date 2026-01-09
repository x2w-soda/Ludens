#include <Ludens/Header/Assert.h>
#include <Ludens/Memory/Memory.h>
#include <Ludens/RenderBackend/LDShaderCompiler.h>
#include <Ludens/RenderBackend/LDShaderParser.h>

#include <iostream>
#include <string>

namespace LD {

static inline void str_write_tok(std::string& str, LDShaderToken* tok)
{
    LD_ASSERT(tok && tok->pos);

    for (int i = 0; i < tok->len; i++)
        str.push_back(tok->pos[i]);
}

static inline void str_write_indent(std::string& str, int indentLevel)
{
    constexpr int spacesPerLevel = 2;

    for (int i = 0; i < spacesPerLevel * indentLevel; i++)
    {
        str.push_back(' ');
    }
}

static void glsl_translation_unit(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_single_decl(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_fn_prototype(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_fn_param_decl(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_fn_definition(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_empty_stmt(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_compound_stmt(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_expr_stmt(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_control_flow_stmt(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_type_specifier(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_type_qualifier(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_struct_specifier(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_struct_decl(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_struct_member(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_array_specifier(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_layout_qualifier(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_layout_qualifier_id(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_storage_qualifier(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_binary_op(LDShaderNode* root, std::string& glsl, int indent);
static void glsl_constant(LDShaderNode* root, std::string& glsl, int indent);

// clang-format off
struct
{
    LDShaderNodeType type;
    void (*callback)(LDShaderNode* root, std::string& glsl, int indent);
} sGLSLTable[] = {
    {LDS_NODE_TRANSLATION_UNIT,    &glsl_translation_unit},
    {LDS_NODE_SINGLE_DECL,         &glsl_single_decl},
    {LDS_NODE_FN_PROTOTYPE,        &glsl_fn_prototype},
    {LDS_NODE_FN_PARAM_DECL,       &glsl_fn_param_decl},
    {LDS_NODE_FN_DEFINITION,       &glsl_fn_definition},
    {LDS_NODE_EMPTY_STMT,          &glsl_empty_stmt},
    {LDS_NODE_COMPOUND_STMT,       &glsl_compound_stmt},
    {LDS_NODE_IF_STMT,             nullptr},
    {LDS_NODE_FOR_STMT,            nullptr},
    {LDS_NODE_WHILE_STMT,          nullptr},
    {LDS_NODE_SWITCH_STMT,         nullptr},
    {LDS_NODE_SWITCH_CASE,         nullptr},
    {LDS_NODE_EXPR_STMT,           &glsl_expr_stmt},
    {LDS_NODE_CONTROL_FLOW_STMT,   &glsl_control_flow_stmt},
    {LDS_NODE_TYPE_SPECIFIER,      &glsl_type_specifier},
    {LDS_NODE_TYPE_QUALIFIER,      &glsl_type_qualifier},
    {LDS_NODE_STRUCT_SPECIFIER,    &glsl_struct_specifier},
    {LDS_NODE_STRUCT_DECL,         &glsl_struct_decl},
    {LDS_NODE_STRUCT_MEMBER,       &glsl_struct_member},
    {LDS_NODE_ARRAY_SPECIFIER,     &glsl_array_specifier},
    {LDS_NODE_LAYOUT_QUALIFIER,    &glsl_layout_qualifier},
    {LDS_NODE_LAYOUT_QUALIFIER_ID, &glsl_layout_qualifier_id},
    {LDS_NODE_STORAGE_QUALIFIER,   &glsl_storage_qualifier},
    {LDS_NODE_INITIALIZER,         nullptr},
    {LDS_NODE_ASSIGNMENT,          nullptr},
    {LDS_NODE_CONDITIONAL,         nullptr},
    {LDS_NODE_LOGICAL_OR,          &glsl_binary_op},
    {LDS_NODE_LOGICAL_XOR,         &glsl_binary_op},
    {LDS_NODE_LOGICAL_AND,         &glsl_binary_op},
    {LDS_NODE_BITWISE_OR,          &glsl_binary_op},
    {LDS_NODE_BITWISE_XOR,         &glsl_binary_op},
    {LDS_NODE_BITWISE_AND,         &glsl_binary_op},
    {LDS_NODE_EQUAL,               &glsl_binary_op},
    {LDS_NODE_RELATIONAL,          &glsl_binary_op},
    {LDS_NODE_SHIFT,               &glsl_binary_op},
    {LDS_NODE_ADD,                 &glsl_binary_op},
    {LDS_NODE_MUL,                 &glsl_binary_op},
    {LDS_NODE_UNARY,               nullptr},
    {LDS_NODE_INDEX,               nullptr},
    {LDS_NODE_POSTFIX,             nullptr},
    {LDS_NODE_CALL,                nullptr},
    {LDS_NODE_VAR,                 nullptr},
    {LDS_NODE_CONSTANT,            glsl_constant},
    {LDS_NODE_ENUM_COUNT,          nullptr},
};
// clang-format on

static void glsl_translation_unit(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_TRANSLATION_UNIT);

    for (LDShaderNode* child = root->lch; child; child = child->next)
    {
        sGLSLTable[child->type].callback(child, glsl, indent);
    }
}

static void glsl_single_decl(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_SINGLE_DECL);

    // declaration type
    LDShaderNode* declType = root->lch;
    sGLSLTable[declType->type].callback(declType, glsl, indent);

    // optional declaration identifier
    LDShaderToken* ident = root->tok;
    if (ident)
    {
        glsl.push_back(' ');
        str_write_tok(glsl, ident);

        LDShaderNode* rch = root->rch;
        if (rch)
        {
            LD_ASSERT(rch->type == LDS_NODE_ARRAY_SPECIFIER || rch->type == LDS_NODE_STRUCT_DECL);
            sGLSLTable[rch->type].callback(rch, glsl, indent);
        }
    }

    glsl += ";\n";
}

static void glsl_fn_prototype(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_FN_PROTOTYPE);

    // function return type
    LDShaderNode* retType = root->lch;
    sGLSLTable[retType->type].callback(retType, glsl, indent);

    glsl.push_back(' ');

    // function name
    str_write_tok(glsl, root->tok);

    // function parameters
    glsl.push_back('(');
    for (LDShaderNode* param = root->rch; param; param = param->next)
    {
        if (param != root->rch)
            glsl += ", ";
        glsl_fn_param_decl(param, glsl, indent);
    }
    glsl.push_back(')');
}

static void glsl_fn_param_decl(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_FN_PARAM_DECL);

    LDShaderNode* paramType = root->lch;
    sGLSLTable[paramType->type].callback(paramType, glsl, indent);

    glsl.push_back(' ');

    str_write_tok(glsl, root->tok);

    LDShaderNode* arraySpec = root->rch;
    if (arraySpec)
    {
        LD_ASSERT(arraySpec->type == LDS_NODE_ARRAY_SPECIFIER);
        glsl_array_specifier(arraySpec, glsl, indent);
    }
}

static void glsl_fn_definition(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_FN_DEFINITION);

    glsl_fn_prototype(root->lch, glsl, indent);
    glsl_compound_stmt(root->rch, glsl, indent);
}

static void glsl_empty_stmt(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_EMPTY_STMT);

    str_write_indent(glsl, indent);
    glsl += ";\n";
}

static void glsl_compound_stmt(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_COMPOUND_STMT);

    glsl += " {\n";
    indent++;

    for (LDShaderNode* child = root->lch; child; child = child->next)
    {
        sGLSLTable[child->type].callback(child, glsl, indent);
    }

    indent--;

    str_write_indent(glsl, indent);
    glsl += "}\n";
}

static void glsl_expr_stmt(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_EXPR_STMT && root->lch);

    str_write_indent(glsl, indent);

    LDShaderNode* expr = root->lch;
    sGLSLTable[expr->type].callback(expr, glsl, indent);

    glsl += ";\n";
}

static void glsl_control_flow_stmt(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_CONTROL_FLOW_STMT);

    str_write_indent(glsl, indent);
    str_write_tok(glsl, root->tok);

    // return statement expr
    LDShaderNode* expr = root->lch;

    if (expr)
    {
        glsl.push_back(' ');
        sGLSLTable[expr->type].callback(expr, glsl, indent);
    }

    glsl += ";\n";
}

static void glsl_type_specifier(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_TYPE_SPECIFIER);

    // dump type qualifiers before type name
    LDShaderNode* qualifier = root->lch;
    if (qualifier)
    {
        LD_ASSERT(qualifier->type == LDS_NODE_TYPE_QUALIFIER);
        glsl_type_qualifier(qualifier, glsl, indent);
    }
    glsl.push_back(' ');

    // data type name
    str_write_tok(glsl, root->tok);

    // TODO: array specifier
}

static void glsl_type_qualifier(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_TYPE_QUALIFIER);

    // this should dispatch each child to either layout_qualifier or storage qualifier
    for (LDShaderNode* qualifier = root->lch; qualifier; qualifier = qualifier->next)
    {
        if (qualifier != root->lch)
            glsl.push_back(' ');

        sGLSLTable[qualifier->type].callback(qualifier, glsl, indent);
    }
}

static void glsl_struct_specifier(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_STRUCT_SPECIFIER);

    glsl += "struct ";

    if (root->tok)
        str_write_tok(glsl, root->tok);

    LDShaderNode* decl = root->lch;
    LD_ASSERT(decl->type == LDS_NODE_STRUCT_DECL);
    glsl_struct_decl(decl, glsl, indent);
}

static void glsl_struct_decl(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_STRUCT_DECL);

    glsl += "{\n";

    for (LDShaderNode* member = root->lch; member; member = member->next)
    {
        LD_ASSERT(member->type == LDS_NODE_STRUCT_MEMBER);

        glsl_struct_member(member, glsl, indent);
        glsl += ";\n";
    }

    glsl += "}\n";
}

static void glsl_struct_member(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_STRUCT_MEMBER);

    LDShaderNode* memberType = root->lch;
    sGLSLTable[memberType->type].callback(memberType, glsl, indent);
    glsl.push_back(' ');
    str_write_tok(glsl, root->tok);

    LDShaderNode* arraySpec = root->rch;
    if (arraySpec)
    {
        LD_ASSERT(arraySpec->type == LDS_NODE_ARRAY_SPECIFIER);
        glsl_array_specifier(arraySpec, glsl, indent);
    }
}

static void glsl_array_specifier(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_ARRAY_SPECIFIER);

    glsl.push_back('[');

    LDShaderNode* arrSize = root->lch;
    if (arrSize)
        sGLSLTable[arrSize->type].callback(arrSize, glsl, indent);

    glsl.push_back(']');

    // traverse linked list for array dimensions
    LDShaderNode* arr = root->next;
    if (arr)
    {
        LD_ASSERT(arr->type == LDS_NODE_ARRAY_SPECIFIER);
        glsl_array_specifier(arr, glsl, indent);
    }
}

static void glsl_layout_qualifier(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_LAYOUT_QUALIFIER);

    glsl += "layout(";

    for (LDShaderNode* qualifierID = root->lch; qualifierID; qualifierID = qualifierID->next)
    {
        if (qualifierID != root->lch)
            glsl += ", ";

        glsl_layout_qualifier_id(qualifierID, glsl, indent);
    }

    glsl += ")";
}

static void glsl_layout_qualifier_id(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_LAYOUT_QUALIFIER_ID);

    str_write_tok(glsl, root->tok);

    LDShaderNode* expr = root->lch;
    if (expr)
    {
        glsl += " = ";
        sGLSLTable[expr->type].callback(expr, glsl, indent);
    }
}

static void glsl_storage_qualifier(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_STORAGE_QUALIFIER);

    str_write_tok(glsl, root->tok);
}

static void glsl_binary_op(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root && root->tok && root->lch && root->rch);

    glsl.push_back('(');
    sGLSLTable[root->lch->type].callback(root->lch, glsl, indent);
    glsl.push_back(')');

    str_write_tok(glsl, root->tok);

    glsl.push_back('(');
    sGLSLTable[root->rch->type].callback(root->rch, glsl, indent);
    glsl.push_back(')');
}

static void glsl_constant(LDShaderNode* root, std::string& glsl, int indent)
{
    LD_ASSERT(root->type == LDS_NODE_CONSTANT);

    str_write_tok(glsl, root->tok);
}

struct LDShaderCompilerVulkanObj
{
    std::string vulkanGLSL;

    void generate_vulkan_glsl(LDShaderNode* astRoot);
};

void LDShaderCompilerVulkanObj::generate_vulkan_glsl(LDShaderNode* astRoot)
{
    LD_ASSERT(astRoot && astRoot->type == LDS_NODE_TRANSLATION_UNIT);

    vulkanGLSL = "// generated by LDShaderCompilerVulkan\n";
    glsl_translation_unit(astRoot, vulkanGLSL, 0);
}

LDShaderCompilerVulkan LDShaderCompilerVulkan::create()
{
    LDShaderCompilerVulkanObj* obj = heap_new<LDShaderCompilerVulkanObj>(MEMORY_USAGE_MISC);

    return {obj};
}

void LDShaderCompilerVulkan::destroy(LDShaderCompilerVulkan compiler)
{
    LDShaderCompilerVulkanObj* obj = compiler;

    heap_delete<LDShaderCompilerVulkanObj>(obj);
}

bool LDShaderCompilerVulkan::compile(LDShaderAST ast, std::vector<uint32_t>& spirv)
{
    if (!ast.is_valid())
        return false;

    // NOTE: currently the ldshader source code should map 1:1 to Vulkan GLSL,
    //       once we add specialization constant support, we may need to
    //       manipulate 'constant_id' layout qualifiers in the AST before
    //       mapping it to Vulkan GLSL.
    LDShaderNode* astRoot = ast.get_root();

    mObj->generate_vulkan_glsl(astRoot);
    std::cout << mObj->vulkanGLSL << std::endl;

    // TODO: generate Vulkan GLSL and pass to spirv-cross to get SPIRV
    spirv.clear();
    return false;
}

} // namespace LD