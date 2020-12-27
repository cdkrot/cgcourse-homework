#version 330 core

layout (location = 0) in vec3 in_position;
out vec3 screen_position;

void main() {
    gl_Position = vec4(in_position, 1);
    screen_position = in_position;
}

