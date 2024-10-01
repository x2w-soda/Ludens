#ludens group 0 FrameStatic
#ludens group 1 Viewport
#ludens group 2 Viewport
#ludens group 3 ToneMapping

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
layout (location = 0) out vec4 fColorLDR;

// world viewport should be bound at group 1
layout (group = 1, binding = 1) uniform sampler2D uPosition;
layout (group = 1, binding = 2) uniform sampler2D uNormals;
layout (group = 1, binding = 3) uniform sampler2D uAlbedo;

// screen viewport should be bound at group 2
layout (group = 2, binding = 1) uniform sampler2D uColorHDR;

// LDR result switch and tone mapping parameters
layout (group = 3, binding = 0) uniform Buffer
{
    int LDRResult;
} uBuffer;

void main()
{
    vec2 uv = vTexUV;
#ifdef LD_VULKAN
    uv.y = 1.0 - uv.y;
#endif

    vec3 color;

    switch (uBuffer.LDRResult)
    {
    case 2: // view space normals
        color = texture(uNormals, uv).rgb;
        break;
    case 3: // albedo
        color = texture(uAlbedo, uv).rgb;
        break;
    case 4: // metallic
        color = texture(uNormals, uv).aaa;
        break;
    case 5: // roughness
        color = texture(uPosition, uv).aaa;
        break;
    case 0: // tone mapping with Reinhard operator
    default:
        color = texture(uColorHDR, uv).rgb;
        color = color / (color + vec3(1.0));
    }

    fColorLDR = vec4(color, 1.0);
}