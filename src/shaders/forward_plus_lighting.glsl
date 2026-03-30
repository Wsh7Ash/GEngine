#version 450 core
#extension GL_ARB_shader_storage_buffer_object : enable
#extension GL_EXT_nonuniform_qualifier : enable

// Forward+ Clustered Lighting
// This shader provides functions for fetching lights from clusters
// and computing clustered lighting for forward rendering

// Cluster configuration
#define CLUSTER_SIZE_X 16
#define CLUSTER_SIZE_Y 8
#define CLUSTER_SIZE_Z 16
#define MAX_LIGHTS_PER_CLUSTER 16
#define MAX_LIGHTS 256

// Light types
#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2

// Light data structures
struct ClusteredLight {
    vec3 Position;
    float Range;
    vec3 Color;
    float Intensity;
    vec3 Direction;
    float SpotOuterCone;
    float SpotInnerCone;
    int Type;
    float padding;
};

// Storage buffers for Forward+
layout(std430, binding = 0) buffer LightBuffer {
    ClusteredLight lights[];
};

layout(std430, binding = 1) buffer ClusterLightIndices {
    int lightIndices[];  // Flattened: [clusterIndex * MAX_LIGHTS_PER_CLUSTER + i]
};

layout(std430, binding = 2) buffer ClusterLightCounts {
    int lightCountPerCluster[];
};

layout(binding = 0) uniform sampler2D depthTexture;

// Cluster bounds (view space)
layout(binding = 1) uniform sampler3D clusterBoundsTexture;

// Uniforms
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 inverseProjectionMatrix;
uniform vec3 cameraPos;
uniform int screenWidth;
uniform int screenHeight;
uniform int clusterCountX;
uniform int clusterCountY;
uniform int clusterCountZ;
uniform float nearPlane;
uniform float farPlane;

// Constants
const float PI = 3.14159265359;

// Calculate cluster index from screen coordinates
int getClusterIndex(ivec3 clusterCoord) {
    return clusterCoord.z * clusterCountX * clusterCountY + 
           clusterCoord.y * clusterCountX + 
           clusterCoord.x;
}

ivec3 getClusterCoord(vec2 screenPos, float depth) {
    float zNorm = (depth - nearPlane) / (farPlane - nearPlane);
    
    int z = int(zNorm * clusterCountZ);
    z = clamp(z, 0, clusterCountZ - 1);
    
    int x = int(screenPos.x * clusterCountX);
    int y = int(screenPos.y * clusterCountY);
    x = clamp(x, 0, clusterCountX - 1);
    y = clamp(y, 0, clusterCountY - 1);
    
    return ivec3(x, y, z);
}

// Get number of lights in a cluster
int getClusterLightCount(int clusterIndex) {
    if (clusterIndex < 0 || clusterIndex >= clusterCountX * clusterCountY * clusterCountZ) {
        return 0;
    }
    return lightCountPerCluster[clusterIndex];
}

// Get light index from cluster at offset
int getClusterLightIndex(int clusterIndex, int offset) {
    if (offset < 0 || offset >= MAX_LIGHTS_PER_CLUSTER) {
        return -1;
    }
    int index = clusterIndex * MAX_LIGHTS_PER_CLUSTER + offset;
    if (index >= lights.length()) {
        return -1;
    }
    return lightIndices[index];
}

// Get light data
ClusteredLight getLight(int lightIndex) {
    return lights[lightIndex];
}

// Calculate attenuation for point/spot lights
float calculateAttenuation(vec3 worldPos, ClusteredLight light) {
    float distance = length(light.Position - worldPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    
    if (light.Range > 0.0) {
        attenuation *= clamp(1.0 - (distance / light.Range), 0.0, 1.0);
    }
    
    return attenuation;
}

// Calculate spotlight factor
float calculateSpotFactor(vec3 lightToPos, ClusteredLight light) {
    float theta = dot(normalize(lightToPos), normalize(-light.Direction));
    float outerCone = cos(radians(light.SpotOuterCone));
    float innerCone = cos(radians(light.SpotInnerCone));
    
    float epsilon = innerCone - outerCone;
    float intensity = clamp((theta - outerCone) / epsilon, 0.0, 1.0);
    
    return intensity;
}

// Compute lighting from a single clustered light
vec3 computeClusteredLight(vec3 worldPos, vec3 normal, vec3 viewDir, 
                           vec3 albedo, float metallic, float roughness,
                           ClusteredLight light, vec3 F0) {
    vec3 L;
    float attenuation = 1.0;
    
    if (light.Type == LIGHT_TYPE_DIRECTIONAL) {
        L = normalize(-light.Direction);
    } else if (light.Type == LIGHT_TYPE_POINT) {
        L = normalize(light.Position - worldPos);
        attenuation = calculateAttenuation(worldPos, light);
    } else if (light.Type == LIGHT_TYPE_SPOT) {
        L = normalize(light.Position - worldPos);
        attenuation = calculateAttenuation(worldPos, light);
        attenuation *= calculateSpotFactor(light.Position - worldPos, light);
    }
    
    if (attenuation <= 0.0) return vec3(0.0);
    
    vec3 H = normalize(viewDir + L);
    
    vec3 radiance = light.Color * light.Intensity * attenuation;
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, viewDir), 0.0), F0);
    
    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    
    float NdotL = max(dot(normal, L), 0.0);
    
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// PBR functions (duplicated for standalone use)
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / max(denom, 0.001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// Main clustered lighting function - call this from PBR shader
vec3 computeClusteredLighting(vec3 worldPos, vec3 normal, vec3 viewDir,
                             vec3 albedo, float metallic, float roughness,
                             vec2 screenCoord, float depth) {
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // Get cluster coordinates
    vec2 ndc = screenCoord * 2.0 - 1.0;
    float linearDepth = (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - ndc.y * (farPlane - nearPlane));
    ivec3 clusterCoord = getClusterCoord(screenCoord, linearDepth);
    
    int clusterIndex = getClusterIndex(clusterCoord);
    int lightCount = getClusterLightCount(clusterIndex);
    
    vec3 Lo = vec3(0.0);
    
    for (int i = 0; i < lightCount && i < MAX_LIGHTS_PER_CLUSTER; i++) {
        int lightIndex = getClusterLightIndex(clusterIndex, i);
        if (lightIndex < 0 || lightIndex >= lights.length()) continue;
        
        ClusteredLight light = getLight(lightIndex);
        Lo += computeClusteredLight(worldPos, normal, viewDir, albedo, metallic, roughness, light, F0);
    }
    
    return Lo;
}
