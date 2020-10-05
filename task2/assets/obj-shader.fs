#version 330 core

in vec3 coordinates;
in vec3 normal;
out vec4 o_frag_color;

uniform vec3 u_camera;
uniform vec4 u_color;
uniform float u_reflect;
uniform float u_refract;
uniform float u_refract_coeff;
uniform samplerCube u_tex;

void main() {
    o_frag_color = u_reflect * texture(u_tex, reflect(coordinates - u_camera, normal))
                 + u_refract * texture(u_tex, refract(coordinates - u_camera, normal, u_refract_coeff))
                 + (1.0f - u_reflect - u_refract) * u_color;
}
