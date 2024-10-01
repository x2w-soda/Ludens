#ludens group 0 Viewport
#ludens group 1 SSAO

#ludens vertex
#version 450 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexUV;
layout (location = 0) out vec2 vTexUV;

void main()
{
	vTexUV = aTexUV;
	gl_Position = vec4(aPos, 0.0, 1.0);
}

#ludens fragment
#version 450 core

layout (location = 0) in vec2 vTexUV;
layout (location = 0) out float fOcclusion;

layout (group = 0, binding = 0, std140) uniform ViewportUBO
{
	mat4 ViewMat;
	mat4 ProjMat;
	mat4 ViewProjMat;
	vec3 viewP;
	vec2 Extent;
	int PointLightStart;
	int PointLightCount;
} uViewportUBO;

layout (group = 0, binding = 1) uniform sampler2D uGBufferPosition;
layout (group = 0, binding = 2) uniform sampler2D uGBufferNormal;

#define KERNEL_SIZE 64

layout (group = 1, binding = 0, std140) uniform Kernel
{
	vec4 Samples[KERNEL_SIZE];
} uKernel;

layout (group = 1, binding = 1) uniform sampler2D uNoise;

vec4 FrameBufferTexture(sampler2D fbt, vec2 uv)
{
#ifdef LD_VULKAN
	uv.y = 1.0 - uv.y;
#endif
	return texture(fbt, uv);
}

void main()
{
	vec2 noiseScale = uViewportUBO.Extent / 16.0;
	vec3 noise = normalize(texture(uNoise, vTexUV * noiseScale).xyz * 2.0 - 1.0);
	vec3 viewP = FrameBufferTexture(uGBufferPosition, vTexUV).xyz;
	vec3 viewN = FrameBufferTexture(uGBufferNormal, vTexUV).xyz;

	// assuming GBuffer pass uses black color to clear normal attachment
	if (length(viewN) < 1e-5)
	{
		fOcclusion = 1.0;
		return;
	}

	viewN = normalize(viewN);

	// construct TBN to convert tangent space samples into view space
	vec3 T = normalize(noise - viewN * dot(noise, viewN));
	vec3 B = cross(viewN, T);
	mat3 TBN = mat3(T, B, viewN);

	const float radius = 0.25;
	const float bias = 0.025;
	float occlusion = 0.0;

	for (int i = 0; i < KERNEL_SIZE; ++i)
	{
		// depth of the sampled position in view space
		vec3 sampleOffset = TBN * uKernel.Samples[i].xyz;
		vec3 sampleP = viewP + sampleOffset * radius;

		// prepare uv to find actual depth of sampled position
		vec4 offset = vec4(sampleP, 1.0);
		offset = uViewportUBO.ProjMat * offset;
		offset.xyz /= offset.w;
		offset.xyz = offset.xyz * 0.5 + 0.5;

		// compare actual and visible depth of sampled position
		float visibleDepth = FrameBufferTexture(uGBufferPosition, offset.xy).z;
		float actualDepth = sampleP.z;
		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(viewP.z - visibleDepth));
		occlusion += (visibleDepth >= actualDepth + bias ? 1.0 : 0.0) * rangeCheck; 
	}

	fOcclusion = 1.0 - (occlusion / KERNEL_SIZE);
}