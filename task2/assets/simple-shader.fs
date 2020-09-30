#version 330 core

struct vx_output_t {
    vec3 color;
};

in vx_output_t v_out;
uniform sampler2D u_tex;
out vec4 o_frag_color;

void main() {
    vec3 texture = texture(u_tex, v_out.color.xy).rgb;
    o_frag_color = vec4(texture,1.0);
    //o_frag_color = vec4(1.0, 0.0, 0.0, 1.0);
}
