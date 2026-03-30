#version 450 core

layout (location = 0) out vec4 color;

in vec3 v_WorldPos;
in vec2 v_TexCoord;
in mat3 v_TBN;

// Material properties
uniform sampler2D u_AlbedoMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_MetallicMap;
uniform sampler2D u_RoughnessMap;
uniform sampler2D u_AOMap;
uniform sampler2D u_SSAO;
uniform sampler2D u_SSGI;         // Screen Space Global Illumination
uniform sampler2D u_gPosition;    // G-Buffer position
uniform sampler2D u_gAlbedo;      // G-Buffer albedo (RGB) + metallic (A)
uniform sampler2D u_gNormal;      // G-Buffer normal (RGB) + roughness (A)
uniform sampler2D u_gVelocity;    // G-Buffer velocity
uniform bool u_UseGBufferMaterials;
uniform float u_SSGIIntensity;    // SSGI intensity multiplier

uniform vec3 u_AlbedoColor;
uniform float u_Metallic;
uniform float u_Roughness;

// IBL
uniform samplerCube u_IrradianceMap;
uniform samplerCube u_PrefilterMap;
uniform sampler2D   u_BRDFLUT;
uniform bool        u_UseIBL;

// Lighting
#define MAX_LIGHTS 8

struct Light {
    int Type; // 0: Directional, 1: Point
    vec3 Position;
    vec3 Direction;
    vec3 Color;
    float Intensity;
    float Range;
};

uniform Light u_Lights[MAX_LIGHTS];
uniform int u_LightCount;

uniform vec3 u_CameraPos;
uniform sampler2D u_ShadowMap;
uniform mat4 u_LightSpaceMatrix;

const float PI = 3.14159265359;

// ----------------------------------------------------------------------------
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(u_ShadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // calculate bias (based on depth map resolution and slope)
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // keep shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
        shadow = 0.0;
        
    return shadow;
}
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.001); 
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------

void main()
{
    // Fetch and prepare material properties - either from material textures or G-Buffer
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
    vec3 normal;
    
    if (u_UseGBufferMaterials)
    {
        // Read from G-Buffer
        vec4 gAlbedo = texture(u_gAlbedo, v_TexCoord);
        vec4 gNormal = texture(u_gNormal, v_TexCoord);
        
        albedo = gAlbedo.rgb;
        metallic = gAlbedo.a;
        normal = gNormal.rgb * 2.0 - 1.0; // Convert from [0,1] to [-1,1]
        roughness = gNormal.a;
        
        // For AO, we'll use a default value or could store it elsewhere
        ao = 1.0; // Default AO
    }
    else
    {
        // Traditional material textures
        albedo     = texture(u_AlbedoMap, v_TexCoord).rgb * u_AlbedoColor;
        metallic  = texture(u_MetallicMap, v_TexCoord).r * u_Metallic;
        roughness = texture(u_RoughnessMap, v_TexCoord).r * u_Roughness;
        ao        = texture(u_AOMap, v_TexCoord).r;
        normal = texture(u_NormalMap, v_TexCoord).rgb;
    }
    
    // Scale screen-space ambient occlusion
    vec2 screenCoords = gl_FragCoord.xy / textureSize(u_SSAO, 0);
    float ssao = texture(u_SSAO, screenCoords).r;
    ao *= ssao;
    
    // Normal mapping
    normal = normal * 2.0 - 1.0;   
    vec3 N = normalize(v_TBN * normal); 
    
    vec3 V = normalize(u_CameraPos - v_WorldPos);
    
    // Calculate reflectance at normal incidence
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
    
    // Reflectance equation
    vec3 Lo = vec3(0.0);
    
    for(int i = 0; i < u_LightCount; ++i)
    {
        vec3 L;
        float attenuation = 1.0;
        
        if (u_Lights[i].Type == 0) // Directional
        {
            L = normalize(-u_Lights[i].Direction);
        }
        else // Point
        {
            L = normalize(u_Lights[i].Position - v_WorldPos);
            float distance = length(u_Lights[i].Position - v_WorldPos);
            attenuation = 1.0 / (distance * distance);
            
            // Optional range clipping
            if (u_Lights[i].Range > 0.0)
                attenuation *= clamp(1.0 - (distance / u_Lights[i].Range), 0.0, 1.0);
        }
        
        vec3 H = normalize(V + L);
        vec3 radiance = u_Lights[i].Color * u_Lights[i].Intensity * attenuation;
        
        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);      
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
            
        vec3 numerator    = NDF * G * F; 
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; 
        vec3 specular = numerator / denominator;
        
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic;	  
        
        float NdotL = max(dot(N, L), 0.0);        
        
        float shadow = 0.0;
        if (i == 0 && u_Lights[i].Type == 0) // Primary directional light shadow
        {
            vec4 fragPosLightSpace = u_LightSpaceMatrix * vec4(v_WorldPos, 1.0);
            shadow = ShadowCalculation(fragPosLightSpace, N, L);
        }
        
        Lo += (kD * albedo / PI + specular) * radiance * NdotL * (1.0 - shadow);  
    }
    
    // Ambient lighting (IBL or fallback)
    vec3 ambient;
    if (u_UseIBL)
    {
        vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
        
        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - metallic;	  
        
        vec3 irradiance = texture(u_IrradianceMap, N).rgb;
        vec3 diffuse      = irradiance * albedo;
        
        // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
        const float MAX_REFLECTION_LOD = 4.0;
        vec3 R = reflect(-V, N); 
        vec3 prefilteredColor = textureLod(u_PrefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
        vec2 brdf  = texture(u_BRDFLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);
        
        ambient = (kD * diffuse + specular) * ao;
    }
    else
    {
        ambient = vec3(0.03) * albedo * ao;
    }
    
    // Add screen-space global illumination
    vec2 ssgiCoords = gl_FragCoord.xy / textureSize(u_SSGI, 0);
    vec3 ssgiContribution = texture(u_SSGI, ssgiCoords).rgb * u_SSGIIntensity;
    ambient += ssgiContribution;
    
    vec3 result = ambient + Lo;
    
    // HDR tonemapping (Reinhard) and Gamma correction
    result = result / (result + vec3(1.0));
    result = pow(result, vec3(1.0/2.2)); 
    
    color = vec4(result, 1.0);
}
