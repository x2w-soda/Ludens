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
layout (location = 0) out float fSSAOBlur;

layout (group = 1, binding = 2) uniform sampler2D uSSAOInput;

void main()
{
	vec2 uv = vTexUV;
#ifdef LD_VULKAN
	uv.y = 1.0 - uv.y;
#endif

	vec2 texelSize = 1.0 / vec2(textureSize(uSSAOInput, 0));
	float sum = 0.0;
	vec2 offset;

	// naive box blur by averaging 16 diagonal pixels
	for (int x = 1; x <= 2; x++)
	{
		for (int y = 1; y <= 2; y++)
		{
			offset = vec2(float(x), float(y)) * texelSize;
			sum += texture(uSSAOInput, uv + offset).r;
			offset = vec2(float(x), float(-y)) * texelSize;
			sum += texture(uSSAOInput, uv + offset).r;
			offset = vec2(float(-x), float(y)) * texelSize;
			sum += texture(uSSAOInput, uv + offset).r;
			offset = vec2(float(-x), float(-y)) * texelSize;
			sum += texture(uSSAOInput, uv + offset).r;
		}
	}

	fSSAOBlur = sum / 16.0;
}