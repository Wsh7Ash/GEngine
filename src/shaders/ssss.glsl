#version 450 core

layout (location = 0) out vec4 FragColor;

in vec2 v_TexCoord;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedo;
uniform sampler2D gSubsurface;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPos;

uniform float u_Power = 12.0;
uniform float u_Scale = 1.0;
uniform float u_Intensity = 1.0;
uniform float u_Radius = 0.5;
uniform int u_SampleCount = 16;
uniform vec2 viewportSize;

const float PI = 3.14159265359;

float rand(vec2 co)
{
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

vec3 getViewPos(vec2 uv)
{
    vec3 viewPos = texture(gPosition, uv).xyz;
    return viewPos;
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
    vec3 sssColor = subsurface.gb;

    if (thickness <= 0.001)
    {
        FragColor = vec4(0.0);
        return;
    }

    vec3 result = vec3(0.0);
    vec2 texelSize = 1.0 / viewportSize;

    float actualRadius = u_Radius * thickness;
    
    int samples = u_SampleCount;
    float angleStep = 2.0 * PI / float(samples);
    
    for (int i = 0; i < 16; i++)
    {
        if (i >= samples) break;
        
        float angle = float(i) * angleStep;
        float radius = actualRadius * (float(i) / float(samples) + 0.5);
        
        vec2 offset = vec2(cos(angle), sin(angle)) * radius * texelSize * viewportSize;
        vec2 sampleUV = v_TexCoord + offset;
        
        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
            continue;
        
        vec3 samplePos = getViewPos(sampleUV);
        vec3 sampleNormal = getNormal(sampleUV);
        vec3 sampleSSS = getSubsurface(sampleUV);
        
        float dist = length(samplePos - viewPos);
        
        float normalWeight = max(0.0, dot(normal, sampleNormal));
        float distWeight = 1.0 / (1.0 + dist * dist * 0.1);
        
        float sampleThickness = sampleSSS.r;
        
        vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
        vec3 viewDir = normalize(-viewPos);
        
        float scatter = pow(max(0.0, dot(lightDir, -viewDir)), u_Power);
        scatter *= exp(-sampleThickness * u_Scale);
        
        vec3 contribution = sampleSSS.gb * scatter * distWeight * normalWeight;
        
        result += contribution;
    }
    
    result /= float(samples);
    result *= u_Intensity * thickness;
    
    result += sssColor * thickness * 0.1;
    
    FragColor = vec4(result, 1.0);
}
