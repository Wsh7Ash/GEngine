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

uniform vec3 u_AlbedoColor;
uniform float u_Metallic;
uniform float u_Roughness;

// Lighting (Basic single light for now)
uniform vec3 u_CameraPos;
uniform vec3 u_LightPos;
uniform vec3 u_LightColor;

const float PI = 3.14159265359;

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

void main()
{
    // Fetch and prepare material properties
    vec3 albedo     = texture(u_AlbedoMap, v_TexCoord).rgb * u_AlbedoColor;
    float metallic  = texture(u_MetallicMap, v_TexCoord).r * u_Metallic;
    float roughness = texture(u_RoughnessMap, v_TexCoord).r * u_Roughness;
    float ao        = texture(u_AOMap, v_TexCoord).r;

    // Normal mapping
    vec3 normal = texture(u_NormalMap, v_TexCoord).rgb;
    normal = normal * 2.0 - 1.0;   
    vec3 N = normalize(v_TBN * normal); 

    vec3 V = normalize(u_CameraPos - v_WorldPos);

    // Calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 of 0.04 
    // and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // Reflectance equation
    vec3 Lo = vec3(0.0);
    
    // Single light source logic
    vec3 L = normalize(u_LightPos - v_WorldPos);
    vec3 H = normalize(V + L);
    float distance = length(u_LightPos - v_WorldPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = u_LightColor * attenuation;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);   
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);
       
    vec3 numerator    = NDF * G * F; 
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; 
    vec3 specular = numerator / denominator;
    
    // kS is equal to Fresnel
    vec3 kS = F;
    // for energy conservation, the diffuse and specular light can't
    // be above 1.0 (unless the surface emits light); to preserve this
    // relationship the diffuse component (kD) should equal 1.0 - kS.
    vec3 kD = vec3(1.0) - kS;
    // multiply kD by the inverse metalness such that only non-metals 
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    kD *= 1.0 - metallic;	  

    // Scale light by NdotL
    float NdotL = max(dot(N, L), 0.0);        

    // add to outgoing radiance Lo
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;  

    // Ambient lighting (very barebones)
    vec3 ambient = vec3(0.03) * albedo * ao;
    
    vec3 result = ambient + Lo;

    // HDR tonemapping (Reinhard) and Gamma correction
    result = result / (result + vec3(1.0));
    result = pow(result, vec3(1.0/2.2)); 

    color = vec4(result, 1.0);
}
