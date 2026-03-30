#version 450 core
layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;

// G-Buffer textures
uniform sampler2D gPosition;    // World position
uniform sampler2D gNormal;      // World normal
uniform sampler2D gAlbedo;      // Base color (RGB) + metallic (A)
uniform sampler2D texNoise;     // Noise texture for SSAO-style sampling

// Camera and projection
uniform mat4 projection;
uniform mat4 view;
uniform vec3 cameraPos;

// SSGI parameters
uniform int sampleCount = 16;
uniform float radius = 0.5;
uniform float intensity = 1.0;
uniform float bounceIntensity = 0.5; // For multiple bounces (foundation: 1 bounce)

// Tile noise over screen
const vec2 noiseScale = vec2(1280.0/4.0, 720.0/4.0); 

// Function to generate random float in [0,1]
float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{
    // Get input data from G-Buffer
    vec3 fragPos = texture(gPosition, TexCoords).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 albedo = texture(gAlbedo, TexCoords).rgb;
    vec3 randomVec = normalize(texture(texNoise, TexCoords * noiseScale).xyz);

    // Create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    // Iterate over sample hemisphere and calculate indirect lighting
    vec3 indirectLight = vec3(0.0);
    for(int i = 0; i < sampleCount; ++i)
    {
        // Generate sample in hemisphere oriented by normal
        vec2 sample = vec2(rand(TexCoords + float(i)), rand(TexCoords + vec2(1.0, float(i))));
        sample = sample * 2.0 - 1.0; // [-1,1]
        vec3 sampleDir = TBN * vec3(sample, sqrt(1.0 - dot(sample, sample))); // Hemisphere sampling

        // Trace ray to find indirect lighting
        vec3 samplePos = fragPos + sampleDir * radius;
        
        // Project sample position to screen space
        vec4 clipPos = projection * view * vec4(samplePos, 1.0);
        clipPos /= clipPos.w;
        vec2 sampleUV = clipPos.xy * 0.5 + 0.5; // [0,1] UV

        // Check if sample is within screen bounds
        if(sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
            continue;

        // Get sample data from G-Buffer
        vec3 samplePosBuf = texture(gPosition, sampleUV).xyz;
        vec3 sampleNormalBuf = texture(gNormal, sampleUV).rgb;
        vec3 sampleAlbedoBuf = texture(gAlbedo, sampleUV).rgb;

        // Calculate distance to sample point
        float distance = length(samplePosBuf - fragPos);
        
        // Only contribute if sample is in front of surface (avoid self-occlusion)
        if(dot(sampleNormalBuf, -sampleDir) < 0.0)
            continue;

        // Calculate geometric term (simple inverse square falloff)
        float geometric = 1.0 / (distance * distance + 0.001); // Avoid division by zero

        // Calculate visibility (check if sample is occluded)
        float visibility = 1.0;
        if(samplePosBuf.z > texture(gPosition, sampleUV).z + 0.01) // Simple depth check
            visibility = 0.0;

        // Accumulate indirect light (diffuse bounce)
        indirectLight += sampleAlbedoBuf * geometric * visibility * max(0.0, dot(sampleNormalBuf, -sampleDir));
    }

    // Normalize by sample count and apply intensity
    indirectLight = indirectLight / float(sampleCount) * intensity;

    // For foundation phase, we approximate multiple bounces by amplifying the indirect light
    // In a more advanced implementation, we would iterate or use techniques like SVOGI
    indirectLight *= (1.0 + bounceIntensity);

    // Output indirect lighting (to be combined with direct lighting in PBR)
    FragColor = vec4(indirectLight, 1.0);
}