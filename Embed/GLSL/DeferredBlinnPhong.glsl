#ludens group 0 FrameStatic
#ludens group 1 Viewport

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
layout (location = 0) out vec4 fColor;

struct PointLightUBO
{
	vec4 PosRadius;
	vec4 Color;
};

layout (group = 0, binding = 0, std140) uniform LightingUBO
{
	vec4 DirLight;
	vec4 DirLightColor;
	PointLightUBO PointLights[128]; // TODO: upstream from macros
} uLightingUBO;

layout (group = 1, binding = 0, std140) uniform ViewportUBO
{
	mat4 ViewMat;
	mat4 ProjMat;
	mat4 ViewProjMat;
	vec4 ViewPos;
	vec2 Viewport;
	int PointLightStart;
	int PointLightCount;
} uViewportUBO;

layout (group = 1, binding = 1) uniform sampler2D uGBufferPosition;
layout (group = 1, binding = 2) uniform sampler2D uGBufferNormal;
layout (group = 1, binding = 3) uniform sampler2D uGBufferAlbedo;
layout (group = 1, binding = 4) uniform sampler2D uSSAOTexture;

void main()
{
	vec2 uv = vTexUV;

#ifdef LD_VULKAN
	uv.y = 1.0 - uv.y;
#endif

	vec4 albedoSpec = texture(uGBufferAlbedo, uv);
	vec3 position = texture(uGBufferPosition, uv).rgb;
	vec3 normal = texture(uGBufferNormal, uv).rgb;
	vec3 albedo = albedoSpec.rgb;
	float specular = albedoSpec.a;
	float occlusion = texture(uSSAOTexture, uv).r;
	vec3 color;

	// if there is no geometry, return flat albedo color
	if (length(normal) < 1e-5)
	{
		fColor = vec4(albedo, 1.0);
		return;
	}

	// single directional light
	vec3 lightPos;
	vec3 lightColor = uLightingUBO.DirLightColor.rgb;
	vec3 lightDir = normalize((uViewportUBO.ViewMat * vec4(-uLightingUBO.DirLight.xyz, 0.0)).xyz);
	vec3 viewPos = (uViewportUBO.ViewMat * vec4(uViewportUBO.ViewPos.xyz, 1.0)).xyz;;
	vec3 viewDir = normalize(viewPos - position);
	vec3 halfwayDir = normalize(lightDir + viewDir);
	
	// diffuse
	vec3 ambientResult = albedo;
	vec3 diffuseResult = vec3(0.0);
	vec3 specularResult = vec3(0.0);
	
	diffuseResult += max(dot(normal, lightDir), 0.0) * albedo * lightColor;
	specularResult += pow(max(dot(normal, halfwayDir), 0.0), 16.0) * specular * lightColor;

	// point lights
	for (int i = 0; i < uViewportUBO.PointLightCount; i++)
	{
		int idx = uViewportUBO.PointLightStart + i;
		lightPos = (uViewportUBO.ViewMat * vec4(uLightingUBO.PointLights[idx].PosRadius.xyz, 1.0)).xyz;
		lightColor = uLightingUBO.PointLights[idx].Color.rgb;
		lightDir = normalize(lightPos - position);
		halfwayDir = normalize(lightDir + viewDir);

		float distance = length(lightPos - position);
		float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance)); 

		diffuseResult += max(dot(normal, lightDir), 0.0) * albedo * lightColor * attenuation;
		specularResult += pow(max(dot(normal, halfwayDir), 0.0), 16.0) * specular * lightColor;
	}

	ambientResult *= occlusion;
	diffuseResult *= occlusion;
	color = ambientResult * 0.2 + diffuseResult * 0.5 + specularResult * 0.3;

	fColor = vec4(color, 1.0);
}