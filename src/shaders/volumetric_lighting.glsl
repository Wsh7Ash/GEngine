#version 450 core
out vec4 FragColor;

in vec2 TexCoords;

#define MAX_CSM_CASCADES 4

uniform sampler2D u_DepthMap;
uniform sampler2D u_ShadowMaps[MAX_CSM_CASCADES];
uniform mat4 u_InverseViewProj;
uniform mat4 u_LightSpaceMatrices[MAX_CSM_CASCADES];
uniform float u_CSMSplitDepths[MAX_CSM_CASCADES];
uniform int u_CSMCount;
uniform vec3 u_LightPos;
uniform vec3 u_CameraPos;
uniform float u_Scattering = 0.5;
uniform float u_Intensity = 1.0;
uniform int u_Samples = 32;

int GetCascadeIndex(float viewZ) {
    for (int i = 0; i < u_CSMCount - 1; i++) {
        if (viewZ < u_CSMSplitDepths[i]) {
            return i;
        }
    }
    return u_CSMCount - 1;
}

void main()
{
    float depth = texture(u_DepthMap, TexCoords).r;
    vec4 ndc = vec4(TexCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 worldPos = u_InverseViewProj * ndc;
    worldPos /= worldPos.w;

    vec3 startPos = u_CameraPos;
    vec3 endPos = worldPos.xyz;
    vec3 rayDir = endPos - startPos;
    float rayLength = length(rayDir);
    rayDir /= rayLength;

    float stepSize = rayLength / float(u_Samples);
    vec3 stepVec = rayDir * stepSize;

    float volumetricVisibility = 0.0;
    vec3 currentPos = startPos;

    for (int i = 0; i < u_Samples; ++i)
    {
        vec4 viewPos = u_InverseViewProj * vec4(currentPos, 1.0);
        float viewZ = -viewPos.z;
        int cascadeIdx = GetCascadeIndex(viewZ);
        
        vec4 lightSpacePos = u_LightSpaceMatrices[cascadeIdx] * vec4(currentPos, 1.0);
        vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
        projCoords = projCoords * 0.5 + 0.5;

        if (projCoords.x >= 0.0 && projCoords.x <= 1.0 && projCoords.y >= 0.0 && projCoords.y <= 1.0 && projCoords.z <= 1.0) {
            float shadowDepth = texture(u_ShadowMaps[cascadeIdx], projCoords.xy).r;
            if (shadowDepth > projCoords.z - 0.001) {
                volumetricVisibility += 1.0;
            }
        }
        currentPos += stepVec;
    }

    volumetricVisibility /= float(u_Samples);
    FragColor = vec4(vec3(volumetricVisibility * u_Scattering * u_Intensity), 1.0);
}
