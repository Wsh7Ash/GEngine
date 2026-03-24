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

uniform sampler2D u_Texture;
uniform float u_Threshold;

void main() {
    vec4 texColor = texture(u_Texture, v_TexCoord);
    float brightness = dot(texColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > u_Threshold)
        color = vec4(texColor.rgb, 1.0);
    else
        color = vec4(0.0, 0.0, 0.0, 1.0);
}
