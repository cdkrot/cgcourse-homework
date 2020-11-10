#version 330 core

in vec3 coordinates;
in vec3 normal_;
out vec4 o_frag_color;

uniform vec4 u_water_color;
uniform float u_water_level;
uniform vec3 u_sun_direction;
uniform float u_light_ambient;
uniform float u_light_diffuse;
uniform float u_light_wat_diff;
uniform float u_light_wat_spec;
uniform vec3 u_camera;
//uniform vec4 u_color;

void water_main() {
    vec4 source_color = u_water_color;
    
    vec3 color = source_color.xyz;
    vec3 normal = normalize(normal_);

    vec3 to_camera = normalize(u_camera - coordinates);
    vec3 sun_reflects_to = reflect(normalize(-u_sun_direction), normal);
    
    
    float light_factor = u_light_ambient
        + u_light_wat_diff * max(0.f, dot(normal, u_sun_direction))
        + u_light_wat_spec * pow(max(0.f, dot(to_camera, u_sun_direction)), 27);

    o_frag_color = vec4(color * light_factor, source_color.w);
}

void ground_main() {
    vec4 source_color = vec4(0.0, 0.5, 0.0, 1.0); //u_color;
    
    vec3 color = source_color.xyz;
    vec3 normal = normalize(normal_);
    
    float light_factor = u_light_ambient + u_light_diffuse * max(0.f, dot(normal, u_sun_direction));
    
    o_frag_color = vec4(color * light_factor, source_color.w);
}

void main() {
    if (coordinates.y <= u_water_level)
        water_main();
    else
        ground_main();
}
