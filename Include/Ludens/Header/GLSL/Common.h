#pragma once

#define LD_GLSL_ROTATE R"(
mat3 ld_rotate(float radians, vec3 axis) {
    float c = cos(radians);
    float s = sin(radians);
    vec3 t = axis * (1.0 - c);
    return mat3(
        t.x * axis.x + c,          t.x * axis.y + s * axis.z, t.x * axis.z - s * axis.y,
        t.x * axis.y - s * axis.z, t.y * axis.y + c,          t.y * axis.z + s * axis.x,
        t.x * axis.z + s * axis.y, t.y * axis.z - s * axis.x, t.z * axis.z + c
    );
})"

// Normal mapping from tangent space to world space in the fragment shader.
// Without tangent vertex attributes: http://www.thetenthplanet.de/archives/1180
#define LD_GLSL_GET_NORMAL R"(
vec3 get_normal(vec3 worldP, vec3 worldN, vec2 uv, vec3 sampleN) {
	vec3 tangentN = sampleN * 2.0 - 1.0;
	vec3 dp1 = dFdx(worldP);
	vec3 dp2 = dFdy(worldP);
	vec2 duv1 = dFdx(uv);
	vec2 duv2 = dFdy(uv);
	vec3 dp2perp = cross( dp2, worldN );
	vec3 dp1perp = cross( worldN, dp1 );
	vec3 T = dp2perp * duv1.x + dp1perp * duv2.x;
	vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;
	float invmax = inversesqrt(max(dot(T,T), dot(B,B)));
	return normalize(mat3(T * invmax, B * invmax, worldN) * tangentN);
})"
