#version 330 core

in vec3 coordinates;
in vec3 normal_;
out vec4 o_frag_color;

uniform vec4 u_color;
uniform vec4 u_water_color;
uniform float u_water_level;
uniform vec3 u_sun_direction;
uniform float u_light_ambient;
uniform vec3 u_light;
uniform vec3 u_light_wat;
uniform vec3 u_camera;

uniform vec3 u_lighthouse_flash_dir;
uniform vec3 u_lighthouse_location;

vec3 get_shininess(vec3 normal, vec3 light_direction, vec3 to_camera) {
    return vec3(1.0, // ambient
                max(0.f, dot(normal, u_sun_direction)), // diffuse
                pow(max(0.f, dot(to_camera, light_direction)), 27)); // specular
}

vec4 common_main(vec4 source_color, vec3 light_vec) {
    vec3 from_lighthouse = normalize(coordinates - u_lighthouse_location);
    
    vec3 normal = normalize(normal_);
    vec3 to_camera = normalize(u_camera - coordinates);
        
    float light_factor = dot(light_vec,
                             get_shininess(normal, u_sun_direction, to_camera));

    if (dot(from_lighthouse, u_lighthouse_flash_dir) >= 0.95)
        return vec4(source_color.xyz * light_factor, source_color.w) + vec4(0.2,0.2,0.2,0);
    else
        return vec4(source_color.xyz * light_factor, source_color.w);
}

void main() {
    if (coordinates.y <= u_water_level)
        o_frag_color = common_main(u_water_color, u_light_wat);
    else
        o_frag_color = common_main(u_color, u_light);
}
