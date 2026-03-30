#version 450 core
#extension GL_EXT_nonuniform_qualifier : enable

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D u_DepthMap;
uniform sampler2D u_gPosition;
uniform vec3 u_CameraPos;
uniform mat4 u_InverseViewProj;
uniform mat4 u_View;
uniform mat4 u_Projection;

uniform vec3 u_LightDir;
uniform vec3 u_LightColor;
uniform float u_LightIntensity;

uniform int u_CSMCount;
uniform sampler2D u_CSMaps[4];
uniform mat4 u_CSMLightSpaceMatrices[4];
uniform float u_CSMSplitDepths[4];
uniform float u_CSMShadowBias;
uniform float u_CSMBlendWidth;

uniform bool u_FogEnabled;
uniform float u_FogDensity;
uniform float u_FogHeight;
uniform float u_FogHeightFalloff;
uniform float u_FogAnisotropy;
uniform float u_FogMultiScattering;
uniform vec3 u_FogColor;
uniform float u_FogStartDistance;

uniform int u_Samples;
uniform float u_Jitter;

const float PI = 3.14159265359;

float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

float hgPhase(float cosTheta, float g) {
    float g2 = g * g;
    return (1.0 - g2) / (4.0 * PI * pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5));
}

float getFogDensity(vec3 worldPos) {
    float heightFactor = exp(-u_FogHeightFalloff * max(0.0, worldPos.y - u_FogHeight));
    return u_FogDensity * heightFactor;
}

float getFogDensity3D(vec3 worldPos) {
    float baseDensity = getFogDensity(worldPos);
    
    float noise = fract(sin(dot(worldPos.xz, vec2(12.9898, 78.233))) * 43758.5453);
    float noiseVariation = 1.0 + (noise - 0.5) * 0.3;
    
    return baseDensity * noiseVariation;
}

float sampleCSMShadow(vec3 worldPos, vec3 lightDir) {
    float shadowFactor = 1.0;
    float currentDepth = dot(worldPos - u_CameraPos, -lightDir);
    
    for (int i = 0; i < 4; i++) {
        if (i >= u_CSMCount) break;
        
        vec4 lightSpacePos = u_CSMLightSpaceMatrices[i] * vec4(worldPos, 1.0);
        vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
        projCoords = projCoords * 0.5 + 0.5;
        
        if (projCoords.x < 0.0 || projCoords.x > 1.0 || 
            projCoords.y < 0.0 || projCoords.y > 1.0 || 
            projCoords.z > 1.0) {
            continue;
        }
        
        float shadowDepth = texture(u_CSMaps[i], projCoords.xy).r;
        float bias = u_CSMShadowBias;
        float currentShadow = (projCoords.z - bias > shadowDepth) ? 0.0 : 1.0;
        
        if (i < 3) {
            float blendStart = u_CSMSplitDepths[i] * (1.0 - u_CSMBlendWidth);
            float blendEnd = u_CSMSplitDepths[i + 1] * (1.0 + u_CSMBlendWidth);
            float blendFactor = smoothstep(blendStart, blendEnd, currentDepth);
            shadowFactor = mix(shadowFactor, currentShadow, blendFactor);
        } else {
            shadowFactor = currentShadow;
        }
    }
    
    return shadowFactor;
}

vec3 rayMarchFog(vec3 rayOrigin, vec3 rayDir, float maxDist, vec3 lightDir) {
    if (!u_FogEnabled || u_FogDensity <= 0.0) {
        return vec3(0.0);
    }
    
    float jitter = rand(TexCoords + fract(u_Jitter)) * u_Jitter;
    float stepSize = (maxDist - u_FogStartDistance) / float(u_Samples);
    
    vec3 accumLight = vec3(0.0);
    float accumTransmittance = 1.0;
    
    float cosTheta = dot(rayDir, lightDir);
    float phase = hgPhase(cosTheta, u_FogAnisotropy);
    
    float multiScatter = 1.0 + u_FogMultiScattering * (1.0 - phase);
    
    for (int i = 0; i < u_Samples; i++) {
        float t = u_FogStartDistance + (float(i) + jitter) * stepSize;
        vec3 samplePos = rayOrigin + rayDir * t;
        
        float density = getFogDensity3D(samplePos);
        
        if (density > 0.001) {
            float shadow = sampleCSMShadow(samplePos, lightDir);
            
            vec3 lightContrib = u_LightColor * u_LightIntensity * shadow * phase * multiScatter;
            
            float extinction = density * stepSize;
            float sampleTransmittance = exp(-extinction);
            
            accumLight += accumTransmittance * lightContrib * density * stepSize;
            accumTransmittance *= sampleTransmittance;
            
            if (accumTransmittance < 0.01) break;
        }
    }
    
    vec3 fogLight = accumLight * u_FogColor;
    
    float fogAmount = 1.0 - accumTransmittance;
    vec3 ambientFog = u_FogColor * 0.02 * fogAmount;
    
    return fogLight + ambientFog;
}

float linearizeDepth(float depth, float near, float far) {
    return (2.0 * near * far) / (far + near - depth * (far - near));
}

void main() {
    float depth = texture(u_DepthMap, TexCoords).r;
    
    if (depth >= 1.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }
    
    vec4 ndc = vec4(TexCoords * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 worldPos = u_InverseViewProj * ndc;
    worldPos /= worldPos.w;
    
    vec3 rayOrigin = u_CameraPos;
    vec3 rayDir = normalize(worldPos.xyz - rayOrigin);
    
    float nearPlane = 0.1;
    float farPlane = 1000.0;
    float linearDepth = linearizeDepth(depth, nearPlane, farPlane);
    
    vec3 lightDir = normalize(-u_LightDir);
    
    vec3 fogColor = rayMarchFog(rayOrigin, rayDir, linearDepth, lightDir);
    
    FragColor = vec4(fogColor, 1.0);
}
