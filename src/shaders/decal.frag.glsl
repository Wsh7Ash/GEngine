#version 450 core
in vec4 gl_FragCoord; // Fragment coordinates in window space

// G-Buffer attachments as inputs (we'll read/modify them)
layout(location = 0) readonly rgba16f gPosition;     // Position
layout(location = 1) readonly rgba16f gAlbedo;       // Albedo (RGB) + Metallic (A)
layout(location = 2) readonly rgba16f gNormal;       // Normal (RGB) + Roughness (A)
layout(location = 3) readonly rg16f gVelocity;       // Velocity
layout(location = 4) readonly depth_component24f gDepth; // Depth

// Decal textures
uniform sampler2D u_DecalAlbedo;
uniform sampler2D u_DecalNormal;

// Decal parameters
uniform vec4 u_DecalAlbedoColor;   // Default albedo color for decal
uniform vec2 u_DecalMaterial;      // Default material (roughness in X)

// Fade parameters
uniform float u_FadeStart;
uniform float u_FadeEnd;

// Output to G-Buffer (we'll blend with existing values)
layout(location = 0) out vec4 outPosition;
layout(location = 1) out vec4 outAlbedo;
layout(location = 2) out vec4 outNormal;
layout(location = 3) out vec2 outVelocity;

void main()
{
    // Only process if we're within the viewport
    vec2 viewport = vec2(textureSize(gPosition, 0));
    if (gl_FragCoord.x < 0 || gl_FragCoord.x >= viewport.x ||
        gl_FragCoord.y < 0 || gl_FragCoord.y >= viewport.y) {
        discard;
    }
    
    // Read current G-Buffer values
    vec4 currentPosition = texelFetch(gPosition, ivec2(gl_FragCoord), 0);
    vec4 currentAlbedo = texelFetch(gAlbedo, ivec2(gl_FragCoord), 0);
    vec4 currentNormal = texelFetch(gNormal, ivec2(gl_FragCoord), 0);
    vec2 currentVelocity = texelFetch(gVelocity, ivec2(gl_FragCoord), 0);
    float currentDepth = texelFetch(gDepth, ivec2(gl_FragCoord), 0).r;
    
    // Skip if no geometry here (background)
    if (currentDepth <= 0.0) {
        discard;
    }
    
    // Calculate decal influence based on distance from decal center
    // This is a simplified approach - in a full implementation we'd use proper projection
    float distanceFromCenter = length(gl_FragCoord.xy / viewport - 0.5) * 2.0; // 0 to 1
    float fadeFactor = smoothstep(u_FadeStart, u_FadeEnd, distanceFromCenter);
    
    // Sample decal textures (using screen position as UV for simplicity)
    // In a real implementation, we'd project world position onto decal UV space
    vec2 decalUV = gl_FragCoord.xy / viewport;
    vec4 decalAlbedo = texture(u_DecalAlbedo, decalUV);
    vec4 decalNormal = texture(u_DecalNormal, decalUV);
    
    // If decal is transparent, skip
    if (decalAlbedo.a < 0.01) {
        discard;
    }
    
    // Blend decal with existing G-Buffer values
    vec4 newAlbedo = mix(currentAlbedo, decalAlbedo * u_DecalAlbedoColor, fadeFactor);
    vec4 newNormal = mix(currentNormal, decalNormal, fadeFactor);
    
    // Output blended values
    outPosition = currentPosition; // Position doesn't change with decals
    outAlbedo = newAlbedo;
    outNormal = newNormal;
    outVelocity = currentVelocity; // Velocity doesn't change with static decals
}