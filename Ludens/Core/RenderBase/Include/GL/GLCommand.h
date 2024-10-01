#pragma once

#include <glad/glad.h>
#include "Core/Header/Include/Types.h"

namespace LD
{

namespace GLCommand
{

void DrawArrays(GLenum primitive, u32 vertexCount);
void DrawArraysInstanced(GLenum primitive, u32 vertexCount, GLsizei instanceCount);
void DrawArraysInstanced(GLenum primitive, u32 vertexCount, GLsizei instanceCount, GLuint instanceStart);
void DrawElements(GLenum primitive, u32 indexCount, GLenum indexType);
void DrawElementsInstanced(GLenum primitive, u32 indexCount, GLenum indexType, GLsizei instanceCount);
void DrawElementsInstanced(GLenum primitive, u32 indexCount, GLenum indexType, GLsizei instanceCount,
                           GLuint instanceStart);

void DrawElementsInstanced(GLenum primitive, u32 indexCount, GLenum indexType, GLsizei instanceCount, GLuint indexStart,
                           GLuint instanceStart);

} // namespace GLCommand

} // namespace LD