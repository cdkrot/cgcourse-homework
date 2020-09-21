#version 330 core

layout (location = 0) in vec3 in_position;
out vec2 coordinates;

uniform vec2 u_translation;
uniform float u_scale;
uniform float u_aspect_ratio;

void main()
{
    vec2 pos = vec2(in_position.x, in_position.y);

    vec2 tr_pos = pos - u_translation;
    
    gl_Position = vec4(tr_pos.x * u_scale, tr_pos.y * u_scale / u_aspect_ratio, in_position.z, 1.0);
    coordinates = pos;
}
