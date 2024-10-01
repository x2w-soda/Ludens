#ludens group 0 FrameStatic
#ludens group 1 Viewport

#ludens vertex
#version 450 core

layout (location = 0) in vec3 aPos;

layout (group = 1, binding = 0, std140) uniform Viewport
{
	mat4 View;
	mat4 Proj;
	mat4 ViewProj;
	vec4 CameraPos;
} uViewport;

void main()
{
    gl_Position = uViewport.ViewProj * vec4(aPos, 1.0);
}

#ludens fragment
#version 450 core

layout (location = 1) out vec4 fColor;

void main()
{
    fColor = vec4(1.0, 0.0, 0.0, 1.0);
}