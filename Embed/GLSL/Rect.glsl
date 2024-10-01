#ludens group 0 Viewport
#ludens group 1 Rect

#ludens vertex

#version 450 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexUV;
layout (location = 2) in vec4 aColor;
layout (location = 3) in float aTexID;

layout (location = 0) out vec2 vTexUV;
layout (location = 1) out vec4 vColor;
layout (location = 2) out float vTexID;


layout (group = 0, binding = 0, std140) uniform Viewport
{
	mat4 View;
	mat4 Proj;
	mat4 ViewProj;
	vec4 CameraPos;
} uViewport;


void main()
{
	vTexUV = aTexUV;
	vColor = aColor;
	vTexID = aTexID;

	gl_Position = uViewport.ViewProj * vec4(aPos, 0.0, 1.0);
}

#ludens fragment

#version 450 core

layout (location = 0) in vec2 vTexUV;
layout (location = 1) in vec4 vColor;
layout (location = 2) in float vTexID;

layout (location = 0) out vec4 fColor;

layout (group = 1, binding = 0) uniform sampler2D uTexture[16];

void main()
{
	int id = int(vTexID);
	vec4 texel;

	switch (id)
	{
	case  0: texel = texture(uTexture[0], vTexUV); break;
	case  1: texel = texture(uTexture[1], vTexUV); break;
	case  2: texel = texture(uTexture[2], vTexUV); break;
	case  3: texel = texture(uTexture[3], vTexUV); break;
	case  4: texel = texture(uTexture[4], vTexUV); break;
	case  5: texel = texture(uTexture[5], vTexUV); break;
	case  6: texel = texture(uTexture[6], vTexUV); break;
	case  7: texel = texture(uTexture[7], vTexUV); break;
	case  8: texel = texture(uTexture[8], vTexUV); break;
	case  9: texel = texture(uTexture[9], vTexUV); break;
	case 10: texel = texture(uTexture[10], vTexUV); break;
	case 11: texel = texture(uTexture[11], vTexUV); break;
	case 12: texel = texture(uTexture[12], vTexUV); break;
	case 13: texel = texture(uTexture[13], vTexUV); break;
	case 14: texel = texture(uTexture[14], vTexUV); break;
	case 15: texel = texture(uTexture[15], vTexUV); break;
	}

	fColor = texel * vColor;
}