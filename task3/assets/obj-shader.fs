#version 330 core

in vec3 coordinates;
in vec3 normal_;
out vec4 o_frag_color;

uniform vec4 u_color;
uniform vec4 u_water_color;
uniform float u_water_level;
uniform vec3 u_sun_direction;
uniform float u_light_ambient;
uniform float u_light_diffuse;
uniform vec3 u_camera;

void main() {
    vec4 source_color = (coordinates.y <= u_water_level ? u_water_color : u_color);
    
    vec3 color = source_color.xyz;
    vec3 ray = normalize(coordinates - u_camera);
    vec3 normal = normalize(normal_);
    
    float light_factor = u_light_ambient + u_light_diffuse * max(0.f, dot(normal, -ray));
    
    o_frag_color = vec4(color * light_factor, source_color.w);
}
