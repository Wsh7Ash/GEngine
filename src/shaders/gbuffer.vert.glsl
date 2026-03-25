#version 450 core
layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;

layout (std140, binding = 0) uniform CameraData {
    mat4 u_View;
    mat4 u_Projection;
    vec3 u_ViewPos;
};

uniform mat4 u_Model;
uniform mat4 u_PrevModel;
uniform mat4 u_PrevViewProj;

out vec3 FragPos;
out vec3 Normal;
out vec4 PrevPos;
out vec4 CurrentPos;

void main()
{
    vec4 viewPos = u_View * u_Model * vec4(a_Position, 1.0);
    FragPos = viewPos.xyz;
    
    mat3 normalMatrix = transpose(inverse(mat3(u_View * u_Model)));
    Normal = normalMatrix * a_Normal;

    CurrentPos = u_Projection * viewPos;
    PrevPos = u_PrevViewProj * u_PrevModel * vec4(a_Position, 1.0);

    gl_Position = CurrentPos;
}
