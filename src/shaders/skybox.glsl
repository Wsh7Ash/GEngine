#version 450 core
layout (location = 0) in vec3 aPos;

out vec3 v_TexCoord;

uniform mat4 u_Projection;
uniform mat4 u_View;

void main()
{
    v_TexCoord = aPos;
    vec4 pos = u_Projection * u_View * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // Force depth to 1.0
}

// <!-- slide -->
#version 450 core
layout (location = 0) out vec4 color;
in vec3 v_TexCoord;

uniform samplerCube u_Skybox;

void main()
{    
    vec3 envColor = texture(u_Skybox, v_TexCoord).rgb;
    
    // HDR tonemapping and gamma correction (matching pbr.frag)
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2)); 
  
    color = vec4(envColor, 1.0);
}
