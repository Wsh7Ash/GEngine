#version 450 core

out vec4 FragColor;

in vec2 v_TexCoord;

uniform sampler2D tex;
uniform float u_Exposure = 1.0f;
uniform float u_Gamma = 2.2f;
uniform int u_ToneMappingType = 1; // 0=Reinhard, 1=ACES, 2=Filmic, 3=Uncharted2
uniform float u_WhitePoint = 4.0f;

vec3 ToneMapReinhard(vec3 x) {
    return x / (x + vec3(1.0));
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
    vec3 A = vec3(0.15);
    vec3 B = vec3(0.50);
    vec3 C = vec3(0.10);
    vec3 D = vec3(0.20);
    vec3 E = vec3(0.02);
    vec3 F = vec3(0.30);
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 ToneMapUncharted2(vec3 x) {
    vec3 A = vec3(0.15);
    vec3 B = vec3(0.50);
    vec3 C = vec3(0.10);
    vec3 D = vec3(0.20);
    vec3 E = vec3(0.02);
    vec3 F = vec3(0.30);
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec3 ApplyToneMapping(vec3 x, int type) {
    if (type == 0) return ToneMapReinhard(x);
    if (type == 1) return ToneMapACES(x);
    if (type == 2) return ToneMapFilmic(x);
    if (type == 3) return ToneMapUncharted2(x);
    return ToneMapACES(x);
}

void main()
{
    vec3 color = texture(tex, v_TexCoord).rgb;
    
    // Apply exposure
    color *= u_Exposure;
    
    // Tone mapping
    color = ApplyToneMapping(color, u_ToneMappingType);
    
    // Gamma correction
    color = pow(color, vec3(1.0 / u_Gamma));
    
    FragColor = vec4(color, 1.0);
}
