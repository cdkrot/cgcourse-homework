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

uniform sampler2D u_flashtex;

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

    vec4 result = vec4(source_color.xyz * light_factor, source_color.w);
    float threshold = 0.95;
    if (dot(from_lighthouse, u_lighthouse_flash_dir) >= threshold) {
        from_lighthouse *= 1.0 / dot(from_lighthouse, u_lighthouse_flash_dir);

        if ((dot(from_lighthouse, u_lighthouse_flash_dir) <= 0.99))
            return vec4(1,0,0,1);
        if ((dot(from_lighthouse, u_lighthouse_flash_dir) >= 1.01))
            return vec4(1,0,0,1);

        
        vec3 right = normalize(cross(u_lighthouse_flash_dir, vec3(0, 1, 0)));
        vec3 up = normalize(cross(right, u_lighthouse_flash_dir));

        vec4 flashcolor = texture(u_flashtex, vec2(0.5 * (dot(up, from_lighthouse) + 1),
                                                   0.5 * (dot(right, from_lighthouse) + 1)));

        result = 0.8 * result + 0.3 * flashcolor;
    }

    return result;
}

void main() {
    if (coordinates.y <= u_water_level)
        o_frag_color = common_main(u_water_color, u_light_wat);
    else
        o_frag_color = common_main(u_color, u_light);
}
