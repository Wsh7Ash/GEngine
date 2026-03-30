#version 450 core

layout (location = 0) out vec4 FragColor;

in vec2 v_TexCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gSubsurface;
uniform sampler2D u_SceneColor;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPos;

uniform float u_IOR = 1.5;
uniform float u_Thickness = 1.0;
uniform vec3 u_TintColor = vec3(1.0);
uniform float u_Intensity = 1.0;
uniform vec2 viewportSize;

vec3 getViewPos(vec2 uv)
{
    return texture(gPosition, uv).xyz;
}

vec3 getNormal(vec2 uv)
{
    return texture(gNormal, uv).rgb * 2.0 - 1.0;
}

vec3 getSubsurface(vec2 uv)
{
    return texture(gSubsurface, uv).rgb;
}

void main()
{
    vec3 viewPos = getViewPos(v_TexCoord);
    vec3 normal = normalize(getNormal(v_TexCoord));
    vec3 subsurface = getSubsurface(v_TexCoord);
    
    float thickness = subsurface.r;
    vec3 tint = subsurface.gb;
    
    if (thickness <= 0.001)
    {
        FragColor = vec4(0.0);
        return;
    }
    
    vec3 viewDir = normalize(cameraPos - viewPos);
    
    float NdotV = max(dot(normal, viewDir), 0.0);
    
    float iorRatio = 1.0 / u_IOR;
    float refractionRatio = iorRatio;
    
    vec3 refracted = refract(-viewDir, normal, refractionRatio);
    
    vec2 distortion = refracted.xy * (1.0 - NdotV) * u_Intensity;
    
    distortion *= u_Thickness * 0.1;
    
    vec2 distortedUV = v_TexCoord + distortion;
    
    distortedUV = clamp(distortedUV, 0.0, 1.0);
    
    vec3 background = texture(u_SceneColor, distortedUV).rgb;
    
    float absorption = exp(-thickness * u_Thickness * 2.0);
    vec3 absorbed = mix(u_TintColor, vec3(1.0), absorption);
    
    vec3 finalColor = background * absorbed;
    
    float fresnel = pow(1.0 - NdotV, 5.0);
    fresnel = mix(0.04, 1.0, fresnel);
    
    vec3 reflected = texture(u_SceneColor, v_TexCoord + normal.xy * 0.05).rgb;
    
    finalColor = mix(finalColor, reflected, fresnel * 0.5);
    
    finalColor *= tint;
    
    FragColor = vec4(finalColor, thickness);
}
