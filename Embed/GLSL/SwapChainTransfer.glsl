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

layout (group = 1, binding = 2) uniform sampler2D uColorLDR;

void main()
{
    vec2 uv = vTexUV;
#ifdef LD_VULKAN
    uv.y = 1.0 - uv.y;
#endif

    vec3 color = texture(uColorLDR, uv).rgb;

    float gamma = 2.2;
    color = pow(color, vec3(1.0 / gamma));
    fColor = vec4(color, 1.0);
}