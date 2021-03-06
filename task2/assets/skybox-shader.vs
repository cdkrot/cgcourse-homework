#version 330 core

struct vx_output_t {
    vec3 texcords;
};

layout (location = 0) in vec3 in_position;

out vx_output_t v_out;
uniform mat4 u_mvp;

void main() {
    v_out.texcords = in_position;
    gl_Position = u_mvp * vec4(in_position, 1);
}
