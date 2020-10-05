#version 330 core

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec3 in_normal;
out vec3 normal;
out vec3 coordinates;

uniform mat4 u_mvp;

void main() {
    normal = in_normal;
    coordinates = in_position;

    gl_Position = u_mvp * vec4(in_position.x, in_position.y, in_position.z, 1.0);
}
