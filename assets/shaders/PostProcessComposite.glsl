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
uniform float u_Exposure;
uniform float u_Gamma;
uniform float u_BloomIntensity;

void main() {
    vec3 sceneColor = texture(u_SceneTexture, v_TexCoord).rgb;
    vec3 bloomColor = texture(u_BloomTexture, v_TexCoord).rgb;
    
    // Additive blending
    sceneColor += bloomColor * u_BloomIntensity;

    // Exposure Tonemapping
    vec3 mapped = vec3(1.0) - exp(-sceneColor * u_Exposure);
    
    // Gamma Correction
    mapped = pow(mapped, vec3(1.0 / u_Gamma));

    color = vec4(mapped, 1.0);
}
