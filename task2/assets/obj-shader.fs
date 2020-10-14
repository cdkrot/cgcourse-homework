#version 330 core

in vec3 coordinates;
in vec3 normal_;
out vec4 o_frag_color;

uniform vec3 u_camera;
uniform vec4 u_color;
uniform float u_base_color_weight;
uniform float u_refract_coeff;
uniform samplerCube u_tex;

void main() {
    vec3 ray = normalize(coordinates - u_camera);
    vec3 normal = normalize(normal_);

    // based on paper by Bram de Greve
    float coeff_reflect = 0.5;
    if (true) {
        float n1 = 1;
        float n2 = u_refract_coeff;

        float r0 = (n1 - n2) / (n1 + n2);
        r0 = r0 * r0;

        float cosx = -dot(normal, ray);
        float x = 1 - cosx;
        coeff_reflect = r0 + (1 - r0) * x * x * x * x * x;
        if (coeff_reflect < 0)
            coeff_reflect = 0;
        if (coeff_reflect > 1)
            coeff_reflect = 1;
    }

    o_frag_color = (1. - u_base_color_weight) * coeff_reflect * texture(u_tex, reflect(ray, normal))
                 + (1. - u_base_color_weight) * (1 - coeff_reflect) * texture(u_tex, refract(ray, normal, u_refract_coeff))
                 + (u_base_color_weight) * u_color;
}
