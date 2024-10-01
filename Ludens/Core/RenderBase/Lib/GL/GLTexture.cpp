#include "Core/Header/Include/Error.h"
#include "Core/RenderBase/Include/GL/GLTexture.h"
#include "Core/RenderBase/Include/GL/GLContext.h"

namespace LD
{

GLTexture2D::GLTexture2D() : mContext(nullptr)
{
}

GLTexture2D::~GLTexture2D()
{
    LD_DEBUG_ASSERT(mHandle == 0);
}

void GLTexture2D::Startup(GLContext& context, const GLTexture2DInfo& info)
{
    mHandle = CUID<GLTexture2D>::Get();
    mContext = &context;
    mInternalFormat = info.InternalFormat;
    mDataFormat = info.DataFormat;
    mDataType = info.DataType;

    glCreateTextures(GL_TEXTURE_2D, 1, &mTexture);
    Bind(0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, info.AddressModeS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, info.AddressModeT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, info.MinFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, info.MagFilter);

    glTexImage2D(GL_TEXTURE_2D, 0, mInternalFormat, (GLsizei)info.Width, (GLsizei)info.Height, 0, mDataFormat,
                 mDataType, info.Data);
    glGenerateMipmap(GL_TEXTURE_2D);
}

void GLTexture2D::Cleanup()
{
    glDeleteTextures(1, &mTexture);

    mHandle.Reset();
    mContext = nullptr;
}

void GLTexture2D::Bind(int unit)
{
    LD_DEBUG_ASSERT(mContext != nullptr);

    mContext->BindTextureUnit(unit);
    mContext->BindTexture2D(*this);
}

GLTexture2DArray::GLTexture2DArray() : mContext(nullptr)
{
}

GLTexture2DArray::~GLTexture2DArray()
{
    LD_DEBUG_ASSERT(mHandle == 0);
}

void GLTexture2DArray::Startup(GLContext& context, const GLTexture2DArrayInfo& info)
{
    mHandle = CUID<GLTexture2DArray>::Get();
    mContext = &context;
    mWidth = info.Width;
    mHeight = info.Height;
    mLayers = info.Layers;

    LD_DEBUG_ASSERT(info.Data != nullptr);

    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &mTexture);
    Bind(0);

    // TODO: mipmap levels
    glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, info.InternalFormat, mWidth, mHeight, mLayers);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, mWidth, mHeight, mLayers, info.DataFormat, info.DataType,
                    info.Data);
    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
}

void GLTexture2DArray::Cleanup()
{
    glDeleteTextures(1, &mTexture);

    mHandle.Reset();
    mContext = nullptr;
}

void GLTexture2DArray::Bind(int unit)
{
    LD_DEBUG_ASSERT(mContext != nullptr);

    mContext->BindTextureUnit(unit);
    mContext->BindTexture2DArray(*this);
}

GLTextureCube::GLTextureCube() : mContext(nullptr), mTexture(0)
{
}

GLTextureCube::~GLTextureCube()
{
    LD_DEBUG_ASSERT(mHandle == 0);
}

void GLTextureCube::Startup(GLContext& context, const GLTextureCubeInfo& info)
{
    mHandle = CUID<GLTextureCube>::Get();
    mContext = &context;

    LD_DEBUG_ASSERT(info.Data != nullptr);

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &mTexture);
    Bind(0);

    glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, info.InternalFormat, info.Resolution, info.Resolution);

    // TODO: pixel byte size from internal format, relax this constraint for cube maps
    LD_DEBUG_ASSERT(info.InternalFormat == GL_RGBA8);
    int faceSize = info.Resolution * info.Resolution * 4;

    for (int i = 0; i < 6; i++)
    {
        const char* data = (const char*)info.Data + faceSize * i;
        glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 0, 0, info.Resolution, info.Resolution, info.DataFormat,
                        info.DataType, data);
    }

    // hard coded sampler properties for cube maps, parameterize later if necessary.
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void GLTextureCube::Cleanup()
{
    glDeleteTextures(1, &mTexture);

    mHandle.Reset();
    mContext = nullptr;
}

void GLTextureCube::Bind(int unit)
{
    LD_DEBUG_ASSERT(mContext != nullptr);

    mContext->BindTextureUnit(unit);
    mContext->BindTextureCube(*this);
}

} // namespace LD