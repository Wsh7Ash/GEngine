#version 450 core

layout (location = 0) out vec4 FragColor;

in vec2 v_TexCoord;

uniform sampler2D u_Texture;
uniform float u_Time;
uniform float u_Intensity = 0.5;
uniform float u_Speed = 1.0;
uniform float u_Scale = 2.0;
uniform vec3 u_ColorA = vec3(1.0, 0.5, 0.0);
uniform vec3 u_ColorB = vec3(0.0, 0.5, 1.0);
uniform vec3 u_ColorC = vec3(1.0, 0.0, 1.0);
uniform bool u_Enable = true;

vec2 abs2(vec2 v) {
    return vec2(abs(v.x), abs(v.y));
}

vec2 yx(vec2 v) {
    return vec2(v.y, v.x);
}

vec4 xyyx(vec2 v) {
    return vec4(v.x, v.y, v.y, v.x);
}

vec2 cos2(vec2 v) {
    return vec2(cos(v.x), cos(v.y));
}

vec4 sin4(vec4 v) {
    return vec4(sin(v.x), sin(v.y), sin(v.z), sin(v.w));
}

vec4 exp4(vec4 v) {
    return vec4(exp(v.x), exp(v.y), exp(v.z), exp(v.w));
}

vec4 tanh4(vec4 v) {
    return vec4(tanh(v.x), tanh(v.y), tanh(v.z), tanh(v.w));
}

float dot2(vec2 a, vec2 b) {
    return dot(a, b);
}

void main()
{
    vec3 baseColor = texture(u_Texture, v_TexCoord).rgb;
    
    if (!u_Enable || u_Intensity <= 0.0) {
        FragColor = vec4(baseColor, 1.0);
        return;
    }
    
    vec2 resolution = vec2(textureSize(u_Texture, 0));
    vec2 r = resolution;
    float t = u_Time * u_Speed;
    
    vec2 FC = v_TexCoord * r;
    vec2 p = (FC * 2.0 - r) / r.y * u_Scale;
    
    vec2 l = vec2(0.0);
    vec2 i = vec2(0.0);
    vec2 v = p * l;
    
    l += 4.0 - 4.0 * abs2(0.7 - dot2(p, p));
    v = p * l;
    
    vec4 o = vec4(0.0);
    
    for (float iy = 1.0; iy <= 8.0; iy += 1.0) {
        vec2 tmp0 = yx(v) * iy;
        tmp0 += vec2(0.0, iy);
        tmp0 += t;
        tmp0 = cos2(tmp0) / iy + 0.7;
        v += tmp0;
        
        vec4 tmp1 = xyyx(v);
        tmp1 = sin4(tmp1) + 1.0;
        tmp1 *= abs(v.x - v.y);
        o += tmp1;
    }
    
    vec4 tmp3 = vec4(-1.0, 1.0, 2.0, 0.0) * (-p.y);
    tmp3 += (l.x - 4.0);
    tmp3 = exp4(tmp3) * 5.0;
    vec4 plasma = tmp3 / o;
    plasma = tanh4(plasma);
    
    vec3 colorA = u_ColorA;
    vec3 colorB = u_ColorB;
    vec3 colorC = u_ColorC;
    
    vec3 plasmaColor = vec3(
        mix(colorA.r, colorB.r, plasma.x),
        mix(colorB.g, colorC.g, plasma.y),
        mix(colorC.b, colorA.b, plasma.z)
    );
    
    plasmaColor = mix(colorA, colorB, plasma.x);
    plasmaColor = mix(plasmaColor, colorC, plasma.y * 0.5);
    
    vec3 finalColor = mix(baseColor, baseColor + plasmaColor * 2.0, u_Intensity);
    
    FragColor = vec4(finalColor, 1.0);
}
