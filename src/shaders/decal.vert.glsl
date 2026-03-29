#version 450 core
layout (location = 0) in vec3 a_Position;

uniform mat4 u_Model;
uniform mat4 u_Projection;  // Projection matrix from decal component

void main()
{
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    gl_Position = u_Projection * worldPos;
}