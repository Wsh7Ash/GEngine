#version 450 core
layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec2 gVelocity;

in vec3 FragPos;
in vec3 Normal;
in vec4 PrevPos;
in vec4 CurrentPos;

void main()
{    
    gPosition = FragPos;
    gNormal = normalize(Normal);
    
    vec2 currentNDC = CurrentPos.xy / CurrentPos.w;
    vec2 prevNDC = PrevPos.xy / PrevPos.w;
    gVelocity = (currentNDC - prevNDC) * 0.5; // NDC to [0,1] velocity
}
