#version 330 core

struct vx_output_t {
    vec3 texcords;
};

in vx_output_t v_out;
uniform samplerCube u_tex;
out vec4 o_frag_color;

void main() {
    vec3 pix = texture(u_tex, v_out.texcords).rgb;
    o_frag_color = vec4(pix, 1.0);
    //o_frag_color = vec4(0.0, 1.0, 0.0, 0.5);
}
