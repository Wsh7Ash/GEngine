#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoord;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;
// Bone data
layout (location = 9) in ivec4 a_BoneIDs;
layout (location = 10) in vec4 a_Weights;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;
uniform mat4 u_BoneMatrices[100];
uniform bool u_IsAnimated;

out vec3 v_WorldPos;
out vec2 v_TexCoord;
out mat3 v_TBN;

void main()
{
    mat4 skinMatrix = mat4(0.0);
    if (u_IsAnimated) {
        for(int i = 0; i < 4; i++) {
            if(a_BoneIDs[i] == -1) continue;
            skinMatrix += u_BoneMatrices[a_BoneIDs[i]] * a_Weights[i];
        }
    } else {
        skinMatrix = mat4(1.0);
    }

    vec4 localPos = skinMatrix * vec4(a_Position, 1.0);
    vec4 worldPos = u_Model * localPos;
    v_WorldPos = worldPos.xyz;
    v_TexCoord = a_TexCoord;

    // Correct normal matrix for non-uniform scaling
    mat3 normalMatrix = transpose(inverse(mat3(u_Model * skinMatrix)));
    vec3 T = normalize(normalMatrix * a_Tangent);
    vec3 B = normalize(normalMatrix * a_Bitangent);
    vec3 N = normalize(normalMatrix * a_Normal);
    v_TBN = mat3(T, B, N);

    gl_Position = u_ViewProjection * worldPos;
}
