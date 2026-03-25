#version 450 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_DepthMap;
uniform sampler2D u_ShadowMap;
uniform mat4 u_InverseViewProj;
uniform mat4 u_LightSpaceMatrix;
uniform vec3 u_LightPos;
uniform vec3 u_CameraPos;
uniform float u_Scattering = 0.5;
uniform int u_Samples = 32;

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
        vec4 fragPosLightSpace = u_LightSpaceMatrix * vec4(currentPos, 1.0);
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
        projCoords = projCoords * 0.5 + 0.5;

        float shadowDepth = texture(u_ShadowMap, projCoords.xy).r;
        if (shadowDepth > projCoords.z)
        {
            volumetricVisibility += 1.0;
        }
        currentPos += stepVec;
    }

    volumetricVisibility /= float(u_Samples);
    FragColor = vec4(vec3(volumetricVisibility * u_Scattering), 1.0);
}
