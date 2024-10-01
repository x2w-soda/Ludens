#include <iostream>
#include <glad/glad.h>
#include "Core/RenderBase/Include/GL/GLProgram.h"
#include "Core/RenderBase/Include/GL/GLContext.h"
#include "Core/Header/Include/Error.h"
#include "Core/OS/Include/Memory.h"
#include "Core/OS/Include/Exit.h"

namespace LD
{

GLProgram::GLProgram() : mContext(nullptr)
{
}

GLProgram::~GLProgram()
{
    LD_DEBUG_ASSERT(mHandle == 0);
}

void GLProgram::Startup(GLContext& context, const GLProgramInfo& info)
{
    mHandle = CUID<GLProgram>::Get();
    mContext = &context;
    mProgram = glCreateProgram();
    mIsSpirvData = info.IsSpirvData;

    mVSData = info.VertexShaderData;
    mVSSize = info.VertexShaderSize;
    mFSData = info.FragmentShaderData;
    mFSSize = info.FragmentShaderSize;
    LD_DEBUG_ASSERT(mVSData && mFSData);

    bool result;

    if (mIsSpirvData)
    {
        mVS = glCreateShader(GL_VERTEX_SHADER);
        result = CompileShaderBinary(&mVS, GL_VERTEX_SHADER, mVSData, mVSSize);
        LD_DEBUG_ASSERT(result && "GLProgram vertex shader spirv compile error");
        glAttachShader(mProgram, mVS);

        mFS = glCreateShader(GL_FRAGMENT_SHADER);
        result = CompileShaderBinary(&mFS, GL_FRAGMENT_SHADER, mFSData, mFSSize);
        LD_DEBUG_ASSERT(result && "GLProgram fragment shader spirv compile error");
        glAttachShader(mProgram, mFS);
    }
    else
    {
        mVS = glCreateShader(GL_VERTEX_SHADER);
        result = CompileShaderSource(&mVS, GL_VERTEX_SHADER, mVSData, mVSSize);
        LD_DEBUG_ASSERT(result && "GLProgram vertex shader compile error");
        glAttachShader(mProgram, mVS);

        mFS = glCreateShader(GL_FRAGMENT_SHADER);
        result = CompileShaderSource(&mFS, GL_FRAGMENT_SHADER, mFSData, mFSSize);
        LD_DEBUG_ASSERT(result && "GLProgram fragment shader compile error");
        glAttachShader(mProgram, mFS);
    }

    int linkStatus;
    glLinkProgram(mProgram);
    glGetProgramiv(mProgram, GL_LINK_STATUS, &linkStatus);

    LD_DEBUG_CANARY(linkStatus == GL_TRUE,
                    [&](const char*)
                    {
                        char infoLog[512];

                        glGetProgramInfoLog(mProgram, sizeof(infoLog), NULL, infoLog);
                        std::cout << "GLProgram linkage error\n" << infoLog << std::endl;
                        Exit(0);
                    });

    if (mVSData)
        glDeleteShader(mVS);
    if (mFSData)
        glDeleteShader(mFS);

    mContext->BindProgram(*this);
}

void GLProgram::Cleanup()
{
    UID boundProgram = mContext->GetBoundProgram();

    if (boundProgram == (UID)mHandle)
    {
        mContext->UnbindProgram();
    }

    glDeleteProgram(mProgram);

    mHandle.Reset();
    mContext = nullptr;
}

void GLProgram::Bind()
{
    LD_DEBUG_ASSERT(mContext != nullptr);

    mContext->BindProgram(*this);
}

bool GLProgram::SetUniformInt(const std::string& name, int value)
{
    GLint loc = glGetUniformLocation(mProgram, name.c_str());

    if (loc < 0)
        return false;

    glUniform1i(loc, (GLint)value);
    return true;
}

bool GLProgram::SetUniformMat4(const std::string& name, const Mat4& value)
{
    GLint loc = glGetUniformLocation(mProgram, name.c_str());

    if (loc < 0)
        return false;

    glUniformMatrix4fv(loc, 1, GL_FALSE, static_cast<const GLfloat*>(value.GetData()));
    return true;
}

bool GLProgram::CompileShaderSource(GLuint* shader, GLenum stage, const char* data, u32 byteSize)
{
    LD_DEBUG_ASSERT(shader != nullptr);

    const GLint gl_size(byteSize);
    char infoLog[512];
    int compileStatus;

    glShaderSource(*shader, 1, &data, &gl_size);
    glCompileShader(*shader);
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &compileStatus);

    LD_DEBUG_CANARY(compileStatus == GL_TRUE,
                    [&](const char*)
                    {
                        glGetShaderInfoLog(*shader, sizeof(infoLog), NULL, infoLog);
                        std::cout << "GLProgram source compile error at stage " << stage << "\n"
                                  << infoLog << std::endl;
                        std::cout << "Full Shader Code:\n" << data << std::endl;
                        Exit(0);
                    })

    return compileStatus == GL_TRUE;
}

bool GLProgram::CompileShaderBinary(GLuint* shader, GLenum stage, const char* spirv, u32 byteSize)
{
    LD_DEBUG_ASSERT(shader != nullptr);

    char infoLog[512];
    int compileStatus;

    glShaderBinary(1, shader, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, spirv, byteSize);
    glSpecializeShaderARB(*shader, "main", 0, nullptr, nullptr);
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &compileStatus);

    LD_DEBUG_CANARY(compileStatus == GL_TRUE,
                    [&](const char*)
                    {
                        glGetShaderInfoLog(*shader, sizeof(infoLog), NULL, infoLog);
                        std::cout << "GLProgram binary compile error at stage " << stage << "\n"
                                  << infoLog << std::endl;
                        Exit(0);
                    })

    return compileStatus == GL_TRUE;
}

} // namespace LD
