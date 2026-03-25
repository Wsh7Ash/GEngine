#version 450 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D u_CurrentFrame;
uniform sampler2D u_PreviousFrame;
uniform sampler2D u_VelocityMap;
uniform float u_Feedback = 0.9;

void main()
{
    vec2 velocity = texture(u_VelocityMap, TexCoords).rg;
    vec2 prevTexCoords = TexCoords - velocity;
    
    vec3 current = texture(u_CurrentFrame, TexCoords).rgb;
    vec3 previous = texture(u_PreviousFrame, prevTexCoords).rgb;
    
    // Simple neighborhood clamping to reduce ghosting
    vec3 m1 = vec3(0.0), m2 = vec3(0.0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            vec3 neighbor = textureOffset(u_CurrentFrame, TexCoords, ivec2(x, y)).rgb;
            m1 += neighbor;
            m2 += neighbor * neighbor;
        }
    }
    vec3 mu = m1 / 9.0;
    vec3 sigma = sqrt(max(m2 / 9.0 - mu * mu, 0.0));
    vec3 boxMin = mu - 1.0 * sigma;
    vec3 boxMax = mu + 1.0 * sigma;
    
    previous = clamp(previous, boxMin, boxMax);
    
    FragColor = vec4(mix(current, previous, u_Feedback), 1.0);
}
