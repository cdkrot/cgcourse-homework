#version 330 core

in vec3 coordinates;
in vec3 normal_;
out vec4 o_frag_color;

uniform vec4 u_color;

void main() {
    o_frag_color = u_color;
}