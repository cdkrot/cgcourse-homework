#version 330 core

struct vx_output_t {
    vec3 color;
};

layout (location = 0) in vec3 in_position;
// layout (location = 1) in vec3 in_color;

out vx_output_t v_out;
uniform mat4 u_mvp;

void main() {
    //v_out.color = in_color;
    v_out.color = vec3(in_position.x, in_position.y, in_position.z);

    gl_Position = u_mvp * vec4(in_position.x, in_position.y, in_position.z, 1.0);
}
