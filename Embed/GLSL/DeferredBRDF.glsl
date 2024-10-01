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

#define PI 3.1415926536

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
	PointLightUBO PointLights[128];
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

float DistributionGGX(float NdotH, float a)
{
    float a2 = a * a;
    float numerator = a2;
    float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return numerator / denom;
}

float GeometrySchlickGGX(float NdotV, float k)
{
    float numerator = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return numerator / denom;
}

float GeometrySmith(float NdotV, float NdotL, float k)
{
    float ggx1 = GeometrySchlickGGX(NdotL, k);
    float ggx2 = GeometrySchlickGGX(NdotV, k);

    return ggx1 * ggx2;
}

vec3 Fresnel(float HdotV, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - HdotV, 0.0, 1.0), 5.0);
}

vec3 CookTorranceBRDF(vec3 V, vec3 N, vec3 L, vec3 radiance, vec3 albedo, float roughness, float metallic)
{
    vec3 H = normalize(V + L);
    float NdotH = max(dot(N, H), 0.0);
    float HdotV = max(dot(H, V), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    float a = roughness * roughness;
    float D = DistributionGGX(NdotH, a);

    float k = ((roughness + 1.0) * (roughness + 1.0)) / 8.0;
    float G = GeometrySmith(NdotV, NdotL, k);

    vec3 F = Fresnel(HdotV, F0);

    vec3 numerator = D * G * F;
    float denom = 4.0 * NdotV * NdotL + 0.0001;
    vec3 specularResult = numerator / denom;

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    // lambertian diffuse
    vec3 diffuseResult = kD * albedo / PI;

    // output radiance
    return (diffuseResult + specularResult) * radiance * max(NdotL, 0.0);
}

void main()
{
	vec2 uv = vTexUV;
#ifdef LD_VULKAN
	uv.y = 1.0 - uv.y;
#endif

	vec3 albedo = texture(uGBufferAlbedo, uv).rgb;
	vec4 posRoughness = texture(uGBufferPosition, uv);
    vec4 normalMetallic = texture(uGBufferNormal, uv);
    vec3 position = posRoughness.rgb;
	vec3 N = normalMetallic.rgb;
	float occlusion = texture(uSSAOTexture, uv).r;
    float roughness = posRoughness.a;
    float metallic = normalMetallic.a;

    // if there is no geometry, return flat albedo color
    if (length(N) < 1e-5)
    {
        fColor = vec4(albedo, 1.0);
        return;
    }

    // single directional light
    vec3 lightPos;
    vec3 lightColor = uLightingUBO.DirLightColor.rgb;
    vec3 L = normalize((uViewportUBO.ViewMat * vec4(-uLightingUBO.DirLight.xyz, 0.0)).xyz);
    vec3 V = normalize(-position);
    vec3 H = normalize(L + V);
    vec3 Lo = CookTorranceBRDF(V, N, L, lightColor, albedo, roughness, metallic);

    // point lights
    for (int i = 0; i < uViewportUBO.PointLightCount; i++)
    {
        int idx = uViewportUBO.PointLightStart + i;
        lightPos = (uViewportUBO.ViewMat * vec4(uLightingUBO.PointLights[idx].PosRadius.xyz, 1.0)).xyz;
        lightColor = uLightingUBO.PointLights[idx].Color.rgb;
        L = normalize(lightPos - position);

        float distance = length(lightPos - position);
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * (distance * distance)); 
        vec3 radiance = lightColor * attenuation;

        Lo += CookTorranceBRDF(V, N, L, radiance, albedo, roughness, metallic);
    }

    vec3 ambient = vec3(0.3) * albedo;
    vec3 color = ambient + Lo;

    color = Lo;
    fColor = vec4(color, 1.0);
}