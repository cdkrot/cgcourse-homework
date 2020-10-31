#version 330 core

in vec3 coordinates;
in vec3 normal_;
out vec4 o_frag_color;

uniform int u_is_schlick;
uniform vec3 u_camera;
uniform vec4 u_color;
uniform float u_base_color_weight;
uniform float u_refract_coeff;
uniform samplerCube u_tex;

// partially based on paper by Bram de Greve

float sqr(float a) {
    return  a * a;
}

// vec3 RefractRay(vec3 ray, vec3 normal, float n1, float n2) {
//     vec3 ray_parallel = ray - normal * dot(ray, normal);

//     vec3 refracted_parallel = (n1 / n2) * ray_parallel;

//     float tmp = 1 - dot(refracted_parallel, refracted_parallel);
//     if (tmp < 0)
//         return vec3(0,0,0); // no refraction, I guess

//     vec3 refracted_ortogonal = (-normal) * sqrt(tmp);

//     return refracted_ortogonal + refracted_parallel;
// }

    // if (true) {
    //     float n1 = 1;
    //     float n2 = u_refract_coeff;

    //     float r0 = (n1 - n2) / (n1 + n2);
    //     r0 = r0 * r0;

    //     float cosx = -dot(normal, ray);
    //     float x = 1 - cosx;
    //     coeff_reflect = r0 + (1 - r0) * x * x * x * x * x;
    //     if (coeff_reflect < 0)
    //         coeff_reflect = 0;
    //     if (coeff_reflect > 1)
    //         coeff_reflect = 1;
    // }

void main() {
    vec3 ray = normalize(coordinates - u_camera);
    vec3 normal = normalize(normal_);
    vec3 reflected_ray = ray - 2 * normal * dot(normal, ray);
    vec3 refracted_ray = normalize(refract(ray, normal, u_refract_coeff)); //RefractRay(ray, normal, 1, u_refract_coeff);
    
    float coeff_reflectance = 0;
    float n1 = 1;
    float n2 = u_refract_coeff;

    if (dot(refracted_ray, refracted_ray) <= 0.1)
        coeff_reflectance = 1.0; // Total Internal Reflection
    else if (u_is_schlick == 0) {
        float cos_in = -dot(normal, ray);
        float cos_out = -dot(normal, refracted_ray);
            
        float R_ort = sqr((n1 * cos_in - n2 * cos_out) / (n1 * cos_in + n2 * cos_out));
        float R_par = sqr((n2 * cos_in - n1 * cos_out) / (n2 * cos_in + n1 * cos_out));
        
        coeff_reflectance = (R_ort + R_par) / 2;
        
        if (coeff_reflectance < 0 || coeff_reflectance > 1) {
            //discard;
            return;
        }                
    } else {
        float r0 = sqr((n1 - n2) / (n1 + n2));
        float cos_in = -dot(ray, normal);
        float x = 1 - cos_in;
        
        coeff_reflectance = r0 + (1.0 - r0) * x * x * x * x * x;
    }

    o_frag_color = (1. - u_base_color_weight) * coeff_reflectance * texture(u_tex, reflected_ray)
                 + (1. - u_base_color_weight) * (1 - coeff_reflectance) * texture(u_tex, refracted_ray)
                 + (u_base_color_weight) * u_color;
}
