#pragma once

#include <string>
#include <cstdint>
#include <glad/glad.h>
#include "Core/Math/Include/Mat4.h"
#include "Core/OS/Include/UID.h"
#include "Core/Header/Include/Types.h"

namespace LD
{

class GLContext;

struct GLProgramInfo
{
    bool IsSpirvData = false;
    const char* VertexShaderData = nullptr;
    const char* FragmentShaderData = nullptr;
    u32 VertexShaderSize = 0;
    u32 FragmentShaderSize = 0;
};

class GLProgram
{
public:
    GLProgram();
    GLProgram(const GLProgram&) = delete;
    GLProgram(GLProgram&&) = default;
    ~GLProgram();

    GLProgram& operator=(const GLProgram&) = delete;
    GLProgram& operator=(GLProgram&&) = default;

    void Startup(GLContext& context, const GLProgramInfo& info);
    void Cleanup();

    void Bind();
    bool SetUniformInt(const std::string& name, int value);
    bool SetUniformMat4(const std::string& name, const Mat4& value);

    inline UID GetHandle() const
    {
        return (UID)mHandle;
    }

    inline explicit operator UID() const
    {
        return (UID)mHandle;
    }

    inline explicit operator GLuint() const
    {
        return (GLuint)mProgram;
    }

private:
    bool CompileShaderSource(GLuint* shader, GLenum stage, const char* data, u32 byteSize);
    bool CompileShaderBinary(GLuint* shader, GLenum stage, const char* spirv, u32 byteSize);

    CUID<GLProgram> mHandle;
    GLContext* mContext = nullptr;
    const char* mVSData = nullptr;
    const char* mFSData = nullptr;
    u32 mVSSize = 0;
    u32 mFSSize = 0;
    GLuint mVS;
    GLuint mFS;
    GLuint mProgram;
    bool mIsSpirvData;
};

} // namespace LD