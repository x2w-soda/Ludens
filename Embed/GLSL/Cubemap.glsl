#ludens group 0 FrameStatic
#ludens group 1 Viewport
#ludens group 2 Cubemap

#ludens vertex
#version 450 core

layout (location = 0) in vec3 aPos;
layout (location = 0) out vec3 vPos;

layout (group = 1, binding = 0, std140) uniform ViewportUBO
{
	mat4 ViewMat;
	mat4 ProjMat;
	mat4 ViewProjMat;
	vec3 ViewPos;
	vec2 Extent;
	int PointLightStart;
	int PointLightCount;
} uViewportUBO;

void main()
{
	vPos = aPos;
	mat4 viewMat = mat4(mat3(uViewportUBO.ViewMat));
	vec4 pos = uViewportUBO.ProjMat * viewMat * vec4(aPos, 1.0);
	gl_Position = pos.xyww;
}

#ludens fragment
#version 450 core

layout (location = 0) in vec3 vPos;

// outputs to GBuffer albedo color after depth test
layout (location = 2) out vec4 fAlbedoSpec;

layout (group = 2, binding = 0) uniform samplerCube uCubemap;

void main()
{
    fAlbedoSpec.rgb = texture(uCubemap, vPos).rgb;
}