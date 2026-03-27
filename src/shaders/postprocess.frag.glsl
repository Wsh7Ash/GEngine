#version 450 core

out vec4 FragColor;

in vec2 v_TexCoord;

uniform sampler2D tex;

void main()
{
    vec3 color = texture(tex, v_TexCoord).rgb;
    // Basic Filmic Tone Mapping if it's HDR
    color = color / (color + vec3(1.0));
    // Gamma Correction
    color = pow(color, vec3(1.0 / 2.2));
    FragColor = vec4(color, 1.0);
}
