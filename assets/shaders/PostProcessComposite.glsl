#type vertex
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

out vec2 v_TexCoord;

void main() {
    v_TexCoord = a_TexCoord;
    gl_Position = vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 color;

in vec2 v_TexCoord;

uniform sampler2D u_SceneTexture;
uniform sampler2D u_BloomTexture;

// Exposure & Tone Mapping
uniform float u_Exposure;
uniform float u_WhitePoint;
uniform int u_ToneMappingType; // 0=Reinhard, 1=ACES, 2=Filmic, 3=Uncharted2
uniform float u_AutoExposure;

// Color Grading
uniform float u_Gamma;

// Bloom
uniform float u_BloomIntensity;

// Vignette
uniform bool u_VignetteEnabled;
uniform float u_VignetteIntensity;
uniform float u_VignetteSmoothness;

const float PI = 3.14159265359;

// Tone Mapping Functions
vec3 ToneMapReinhard(vec3 x) {
    return x / (x + vec3(1.0));
}

vec3 ToneMapReinhardExtended(vec3 x, float white) {
    return x * (vec3(1.0) + x / (white * white)) / (vec3(1.0) + x);
}

vec3 ToneMapACES(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 ToneMapFilmic(vec3 x) {
    vec3 A = vec3(0.15, 0.15, 0.15);
    vec3 B = vec3(0.50, 0.50, 0.50);
    vec3 C = vec3(0.10, 0.10, 0.10);
    vec3 D = vec3(0.20, 0.20, 0.20);
    vec3 E = vec3(0.02, 0.02, 0.02);
    vec3 F = vec3(0.30, 0.30, 0.30);
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 ToneMapUncharted2(vec3 x) {
    vec3 A = vec3(0.15, 0.15, 0.15);
    vec3 B = vec3(0.50, 0.50, 0.50);
    vec3 C = vec3(0.10, 0.10, 0.10);
    vec3 D = vec3(0.20, 0.20, 0.20);
    vec3 E = vec3(0.02, 0.02, 0.02);
    vec3 F = vec3(0.30, 0.30, 0.30);
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 ApplyToneMapping(vec3 x) {
    if (u_ToneMappingType == 0) {
        return ToneMapReinhard(x);
    } else if (u_ToneMappingType == 1) {
        return ToneMapACES(x);
    } else if (u_ToneMappingType == 2) {
        return ToneMapFilmic(x);
    } else if (u_ToneMappingType == 3) {
        return ToneMapUncharted2(x);
    }
    return ToneMapACES(x); // Default to ACES
}

// Calculate Luminance
float CalcLuminance(vec3 color) {
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

// Vignette
float CalcVignette(vec2 uv, float intensity, float smoothness) {
    uv = uv * 2.0 - 1.0;
    float dist = length(uv);
    return 1.0 - smoothstep(1.0 - smoothness, 1.0, dist * intensity);
}

void main() {
    vec3 sceneColor = texture(u_SceneTexture, v_TexCoord).rgb;
    vec3 bloomColor = texture(u_BloomTexture, v_TexCoord).rgb;
    
    // Apply bloom (additive)
    sceneColor += bloomColor * u_BloomIntensity;
    
    // Apply exposure
    float exposure = u_AutoExposure > 0.0 ? u_AutoExposure : u_Exposure;
    sceneColor *= exposure;
    
    // Tone mapping
    vec3 mapped = ApplyToneMapping(sceneColor);
    
    // Vignette
    if (u_VignetteEnabled) {
        float vignette = CalcVignette(v_TexCoord, u_VignetteIntensity, u_VignetteSmoothness);
        mapped *= vignette;
    }
    
    // Gamma correction
    mapped = pow(mapped, vec3(1.0 / u_Gamma));

    color = vec4(mapped, 1.0);
}
