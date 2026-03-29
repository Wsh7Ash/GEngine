#version 450 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec4 gAlbedo;   // RGB = albedo, A = metallic
layout (location = 2) out vec4 gNormal;   // RGB = normal, A = roughness
layout (location = 3) out vec2 gVelocity;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 PrevPos;
in vec4 CurrentPos;

// Material uniforms
uniform vec4 u_Albedo;    // RGB = albedo color, A = metallic
uniform vec2 u_Material;  // X = roughness, Y = AO (unused for now)

void main()
{    
    gPosition = FragPos;
    gAlbedo = vec4(u_Albedo.rgb, u_Albedo.a); // albedo RGB, metallic A
    gNormal = vec4(normalize(Normal).rgb, u_Material.x); // normal RGB, roughness A
    
    vec2 currentNDC = CurrentPos.xy / CurrentPos.w;
    vec2 prevNDC = PrevPos.xy / PrevPos.w;
    gVelocity = (currentNDC - prevNDC) * 0.5; // NDC to [0,1] velocity
}
