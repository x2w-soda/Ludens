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

float rgb2luma(vec3 rgb)
{
    return dot(rgb, vec3(0.299, 0.587, 0.114));
}

#define LD_FXAA_THRESHOLD_DIFF       0.0312
#define LD_FXAA_THRESHOLD_DARKNESS   0.125
#define LD_FXAA_ITERATIONS           12

void main()
{
    vec2 uv = vTexUV;
    #ifdef LD_VULKAN
        uv.y = 1.0 - uv.y;
    #endif

    // center pixel
    vec3 colorC = texture(uTexture, uv).rgb;
    float lumaC = rgb2luma(colorC);

    // four neighbors
    vec3 lumaD = rgb2luma(textureOffset(uTexture, uv, ivec2(0, -1)).rgb);
    vec3 lumaU = rgb2luma(textureOffset(uTexture, uv, ivec2(0, 1)).rgb);
    vec3 lumaL = rgb2luma(textureOffset(uTexture, uv, ivec2(-1, 0)).rgb);
    vec3 lumaR = rgb2luma(textureOffset(uTexture, uv, ivec2(1, 0)).rgb);

    // minimum and maximum of five
    float lumaMin = min(lumaC,min(min(lumaD,lumaU),min(lumaL,lumaR)));
    float lumaMax = max(lumaC,max(max(lumaD,lumaU),max(lumaL,lumaR)));
    float lumaDiff = lumaMax - lumaMin;

    // if luminance difference is too small or the area is too dark, don't apply AA
    if (lumaDiff < max(LD_FXAA_THRESHOLD_DIFF, lumaMax * LD_FXAA_THRESHOLD_DARKNESS))
    {
        fColor = colorC;
        return;
    }

    // four corners
    float lumaDL = rgb2luma(textureOffset(uTexture, uv, ivec2(-1,-1)).rgb);
    float lumaUR = rgb2luma(textureOffset(uTexture, uv, ivec2(1,1)).rgb);
    float lumaUL = rgb2luma(textureOffset(uTexture, uv, ivec2(-1,1)).rgb);
    float lumaDR = rgb2luma(textureOffset(uTexture, uv, ivec2(1,-1)).rgb);

    // sums of two corners
    float lumaDUCorners = lumaD + lumaU;
    float lumaLRCorners = lumaL + lumaR;
    float lumaLCorners = lumaDL + lumaUL;
    float lumaRCorners = lumaDR + lumaUR;
    float lumaUCorners = lumaUR + lumaUL;
    float luamDCorners = lumaDL + lumaDR;

    // is the edge vertical or horizontal
    float edgeH =  abs(-2.0 * lumaL + lumaLCorners) + abs(-2.0 * lumaC + lumaDUCorners) * 2.0  + abs(-2.0 * lumaR + lumaRCorners);
    float edgeV =  abs(-2.0 * lumaU + lumaUCorners) + abs(-2.0 * lumaC + lumaLRCorners) * 2.0  + abs(-2.0 * lumaD + lumaDCorners);
    bool isHorizontal = (edgeH >= edgeV);

    // select two neighbor texel lumas in the perpendicular direction to the edge.
    float luma1 = isHorizontal ? lumaD : lumaL;
    float luma2 = isHorizontal ? lumaU : lumaR;
    float gradient1 = abs(luma1 - lumaC);
    float gradient2 = abs(luma2 - lumaC);
    bool is1Steepest = gradient1 >= gradient2;

    // Gradient in the corresponding direction, normalized.
    float gradientScaled = 0.25 * max(gradient1, gradient2);

    // Choose the step size (one pixel) according to the edge direction.
    vec2 invSize = 
    float stepUV = isHorizontal ? invSize.y : invSize.x;
    float lumaLocalAvg = 0.0;

    if (is1Steepest)
    {
        stepUV = -stepUV; // reverse direction 
        lumaLocalAvg = 0.5 * (luma1 + lumaC);
    }
    else
    {
        lumaLocalAvg = 0.5 * (luma2 + lumaC);
    }

    // shift UV by half a pixel, now currentUV pixel should be exactly on the edge
    vec2 currentUV = uv;
    if (isHorizontal)
        currentUV.y += stepUV * 0.5;
    else
        currentUV.x += stepUV * 0.5;

    // uv offset along the positive/negative direction of edge
    vec2 offsetUV = isHorizontal ? vec2(invSize.x, 0.0) : vec2(0.0, invSize.y);
    vec2 uvN = currentUV;
    vec2 uvP = currentUV;

    // luma at edge endpoints, endpoints are determined by local contrast
    float lumaEndN;
    float lumaEndP;
    bool reachedN = 0;
    bool reachedP = 0;

    // step along edge in both directions to find two endpoints
    for (int i = 0; i < LD_FXAA_ITERATIONS; i++)
    {
        if (reachedN && reachedP)
            break;

        if (!reachedN)
        {
            uvN -= offsetUV;
            lumaEndN = rgb2luma(texture(uTexture, uvN).rgb);
        }

        if (!reachedP)
        {
            uvP += offsetUV;
            lumaEndP = rgb2luma(texture(uTexture, uvP).rgb);
        }

        reachedN = reachedN || (abs(lumaEndN - lumaLocalAvg) >= gradientScaled);
        reachedP = reachedP || (abs(lumaEndP - lumaLocalAvg) >= gradientScaled);
    }

    float lengthN = isHorizontal ? (uv.x - uvN.x) : (uv.y - uvN.y);
    float lengthP = isHorizontal ? (uvP.x - uv.x) : (uvP.y - uv.y);
    float edgeLength = lengthN + lengthP;

    // compute the final UV coordinates.
    vec2 finalUV = uv;

    if (isHorizontal)
        finalUV.y += finalOffset * stepUV;
    else
        finalUV.x += finalOffset * stepUV;

    fColor = texture(uTexture, finalUV);
}