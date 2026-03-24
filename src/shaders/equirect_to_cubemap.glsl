#version 450 core
layout (location = 0) in vec3 aPos;

out vec3 v_WorldPos;

uniform mat4 u_Projection;
uniform mat4 u_View;

void main()
{
    v_WorldPos = aPos;
    gl_Position =  u_Projection * u_View * vec4(v_WorldPos, 1.0);
}

// <!-- slide -->
#version 450 core
layout (location = 0) out vec4 color;
in vec3 v_WorldPos;

uniform sampler2D u_EquirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleEquirectangularMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{		
    vec2 uv = SampleEquirectangularMap(normalize(v_WorldPos));
    vec3 envColor = texture(u_EquirectangularMap, uv).rgb;
    color = vec4(envColor, 1.0);
}
