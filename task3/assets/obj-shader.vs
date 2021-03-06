#version 330 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec3 in_color;
out vec3 normal_;
out vec3 coordinates;
out vec3 vs_color;

uniform mat4 u_mvp;

void main() {
    normal_ = in_normal;
    coordinates = in_position;
    vs_color = in_color;

    gl_Position = u_mvp * vec4(in_position.x, in_position.y, in_position.z, 1.0);
}
