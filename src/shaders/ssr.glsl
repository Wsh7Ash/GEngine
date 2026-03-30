#version 450 core
layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;

// G-Buffer textures
uniform sampler2D gPosition;    // View space position
uniform sampler2D gNormal;      // View space normal
uniform sampler2D gAlbedo;      // Albedo (RGB) and metallic (A)

// Camera and projection
uniform mat4 projection;
uniform mat4 view;
uniform mat4 invView;
uniform mat4 invProj;
uniform vec3 cameraPos;

// SSR parameters
uniform int steps = 32;
uniform float stepSize = 0.5;
uniform float fadeDistance = 1.0;
uniform float thickness = 0.01;
uniform float roughnessFade = 1.0;
uniform vec2 viewportSize; // Width and height of the viewport

// Function to generate random float in [0,1] based on fragment coordinates
float rand(vec2 co){
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    // Get input data from G-Buffer
    vec3 viewPos = texture(gPosition, TexCoords).xyz;
    vec3 viewNormal = normalize(texture(gNormal, TexCoords).rgb);
    vec3 albedo = texture(gAlbedo, TexCoords).rgb;

    // Calculate view direction (from fragment to camera)
    vec3 viewDir = normalize(cameraPos - viewPos);

    // Calculate reflection vector
    vec3 reflectDir = reflect(-viewDir, viewNormal);

    // Check if we are facing away (no reflection)
    if (dot(viewNormal, reflectDir) < 0.0) {
        FragColor = vec4(0.0);
        return;
    }

    // Raymarching parameters
    float currentStep = 0.0;
    vec3 marchPos = viewPos;
    vec3 marchDir = reflectDir * stepSize;

    // Initialize color and closest depth
    vec3 accumulatedColor = vec3(0.0);
    float accumulatedWeight = 0.0;

    // Raymarch loop
    for (int i = 0; i < steps; i++) {
        // Current position in view space
        marchPos = viewPos + marchDir * currentStep;

        // Convert view space position to clip space
        vec4 clipPos = projection * view * vec4(marchPos, 1.0);
        if (clipPos.w == 0.0) continue; // Avoid division by zero
        clipPos /= clipPos.w;

        // Convert NDC to texture coordinates [0,1]
        vec2 ssTexCoords = clipPos.xy * 0.5 + 0.5;

        // Check if we are outside the screen
        if (ssTexCoords.x < 0.0 || ssTexCoords.x > 1.0 ||
            ssTexCoords.y < 0.0 || ssTexCoords.y > 1.0) {
            break;
        }

        // Get depth from G-Buffer at the marched position
        float sampleDepth = texture(gPosition, ssTexCoords).z;

        // Check if we have hit a surface (if our marched position is in front of the buffer depth)
        if (marchPos.z > sampleDepth - thickness) {
            // Calculate the weight based on distance and angle
            float distance = length(marchPos - viewPos);
            float weight = 1.0 / (1.0 + distance * distance * 0.1);
            weight *= max(0.0, dot(viewNormal, -marchDir)); // Fresnel-like term

            // Get the albedo at the hit point
            vec3 sampleAlbedo = texture(gAlbedo, ssTexCoords).rgb;

            // Accumulate color
            accumulatedColor += sampleAlbedo * weight;
            accumulatedWeight += weight;

            // Early exit if we have enough contribution
            if (accumulatedWeight > 0.9) {
                break;
            }
        }

        // Increase step
        currentStep += 1.0;
    }

    // Normalize accumulated color
    if (accumulatedWeight > 0.0) {
        accumulatedColor /= accumulatedWeight;
    }

    // Fade out based on distance to avoid harsh edges
    float fade = clamp(1.0 - length(viewPos - cameraPos) / fadeDistance, 0.0, 1.0);
    accumulatedColor *= fade;

    // Apply roughness fade: less SSR contribution on rough surfaces
    // Note: We don't have roughness in the SSR pass, but we can use the albedo's alpha? 
    // Actually, in our G-buffer, roughness is stored in the alpha of gNormal.
    // However, we are not sampling gNormal for roughness here. We'll pass roughness as a uniform? 
    // Alternatively, we can sample gNormal in the SSR pass for roughness. Let's do that.
    // But note: we are already using gNormal for the normal. We can get roughness from the same texture.
    float roughness = texture(gNormal, TexCoords).a;
    accumulatedColor *= (1.0 - roughness * roughnessFade);

    // Output the SSR color
    FragColor = vec4(accumulatedColor, 1.0);
}