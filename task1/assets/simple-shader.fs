#version 330 core

out vec4 o_frag_color;

struct vx_output_t
{
    vec3 color;
};

in vx_output_t v_out;

uniform vec3 u_color;
uniform float u_time;

void main()
{
    float animation = 0.5 + sin(5 * u_time) * sin(5 * u_time);
    o_frag_color = vec4(animation * v_out.color * u_color,1.0);
}
