#ludens group 0 Viewport
#ludens group 1 Material

#ludens vertex

#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec3 aTangent;
layout (location = 3) in vec2 aTexUV;
layout (location = 4) in vec4 aModelMat[3];
layout (location = 7) in vec4 aNormalMat[3];

layout (location = 0) out vec3 vPos;
layout (location = 1) out vec3 vNormal;
layout (location = 2) out vec2 vTexUV;
layout (location = 3) out mat3 vTBN;


layout (group = 0, binding = 0, std140) uniform Viewport
{
	mat4 View;
	mat4 Proj;
	mat4 ViewProj;
	vec4 CameraPos;
} uViewport;

void main()
{
	mat4 modelMat; 
	modelMat[0] = vec4(aModelMat[0].x, aModelMat[1].x, aModelMat[2].x, 0.0);
	modelMat[1] = vec4(aModelMat[0].y, aModelMat[1].y, aModelMat[2].y, 0.0);
	modelMat[2] = vec4(aModelMat[0].z, aModelMat[1].z, aModelMat[2].z, 0.0);
	modelMat[3] = vec4(aModelMat[0].w, aModelMat[1].w, aModelMat[2].w, 1.0);

	mat3 normalMat = mat3(aNormalMat[0].xyz, aNormalMat[1].xyz, aNormalMat[2].xyz);

	vPos = (uViewport.View * modelMat * vec4(aPos, 1.0)).xyz;   // view space position
	vNormal = normalize(normalMat * aNormal);                   // view space normal
	vTexUV = aTexUV;

	vec3 T = normalize(normalMat * aTangent);
	vec3 N = vNormal;
	vec3 B = cross(N, T);
	vTBN = mat3(T, B, N);

	gl_Position = uViewport.Proj * vec4(vPos, 1.0);
}

#ludens fragment


#version 450 core
layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexUV;
layout (location = 3) in mat3 vTBN;

layout (location = 0) out vec4 fPosRoughness;
layout (location = 1) out vec4 fNormalMetallic;
layout (location = 2) out vec4 fAlbedo;

layout (group = 0, binding = 0, std140) uniform Viewport
{
	mat4 View;
	mat4 Proj;
	mat4 ViewProj;
	vec4 CameraPos;
} uViewport;

layout (group = 1, binding = 0, std140) uniform Material
{
	int UseAlbedoTexture;
	int UseNormalTexture;
	float Roughness;
	float Metallic;
	vec4 Albedo;
	int MetallicRoughnessLayout;
} uMaterial;

layout (group = 1, binding = 1) uniform sampler2D uAlbedo;
layout (group = 1, binding = 2) uniform sampler2D uNormal;
layout (group = 1, binding = 3) uniform sampler2D uMetallic;
layout (group = 1, binding = 4) uniform sampler2D uRoughness;

void main()
{
	vec4 albedo = uMaterial.Albedo;
	vec3 normal = normalize(vNormal);

	if (uMaterial.UseAlbedoTexture > 0)
	{
		albedo = texture(uAlbedo, vTexUV);
	}

	if (uMaterial.UseNormalTexture > 0)
	{
		// normal mapping from tangent space to view space
		normal = texture(uNormal, vTexUV).rgb;
		normal = normalize(normal * 2.0 - 1.0);   
		normal = normalize(vTBN * normal);
	}

	float metallic = uMaterial.Metallic;
	float roughness = uMaterial.Roughness;

	switch (uMaterial.MetallicRoughnessLayout)
	{
	case 1: // separate textures
		metallic = texture(uMetallic, vTexUV).r;
		roughness = texture(uRoughness, vTexUV).r;
		break;
	case 2: // single texture
		metallic = texture(uMetallic, vTexUV).b;
		roughness = texture(uMetallic, vTexUV).g;
		break;
	case 3: // metallic only
		metallic = texture(uMetallic, vTexUV).r;
		break;
	case 4: // roughness only
		roughness = texture(uRoughness, vTexUV).r;
		break;
	default: // no textures
		break;
	}

	fPosRoughness = vec4(vPos, roughness);
	fNormalMetallic = vec4(normal, metallic);
	fAlbedo = albedo;
}
