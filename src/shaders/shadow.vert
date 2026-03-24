#version 450 core
layout (location = 0) in vec3 a_Position;
layout (location = 9) in ivec4 a_BoneIDs;
layout (location = 10) in vec4 a_Weights;

uniform mat4 u_LightSpaceMatrix;
uniform mat4 u_Model;
uniform mat4 u_BoneMatrices[100];
uniform bool u_IsAnimated;

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

    gl_Position = u_LightSpaceMatrix * u_Model * skinMatrix * vec4(a_Position, 1.0);
}
