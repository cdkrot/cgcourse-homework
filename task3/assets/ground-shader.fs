#version 330 core

in vec3 coordinates;
in vec3 normal_;
out vec4 o_frag_color;

uniform vec4 u_color;
uniform vec4 u_water_color;
//#define u_color vec4(0.0, 0.8, 0.1, 1.0)
//#define u_water_color vec4(0.1, 0.3, 1.0, 1.0)

uniform float u_water_level;
uniform vec3 u_sun_location;
uniform float u_light_ambient;
uniform vec3 u_light;
uniform vec3 u_light_wat;
uniform vec3 u_camera;

uniform vec3 u_lighthouse_flash_dir;
uniform vec3 u_lighthouse_location;

uniform mat4 u_lightmat;
uniform sampler2D u_shadowmap;
uniform sampler2D u_flashtex;

vec3 get_shininess(vec3 normal, vec3 light_direction, vec3 to_camera, bool shadow) {
    vec3 res = vec3(1.0, // ambient
                    max(0.f, dot(normal, light_direction)), // diffuse
                    pow(max(0.f, dot(to_camera, reflect(normal, light_direction))), 27)); // specular

    if (shadow) {
        res.z = 0;
        res.xy *= 0.3;
    }
    return res;
}

vec4 common_main(vec4 source_color, vec3 light_vec, bool shadow) {
    vec3 from_lighthouse = normalize(coordinates - u_lighthouse_location);
    
    vec3 normal = normalize(normal_);
    vec3 to_camera = normalize(u_camera - coordinates);
    vec3 to_sun = normalize(u_sun_location - coordinates);
    
    float light_factor = dot(light_vec,
                             get_shininess(normal, to_sun, to_camera, shadow));

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
    vec4 shadowmap_proj_ = u_lightmat * vec4(coordinates, 1);
    vec3 shadowmap_proj = shadowmap_proj_.xyz / shadowmap_proj_.w;
    shadowmap_proj = 0.5 * (shadowmap_proj + vec3(1,1,1));

    //o_frag_color = vec4(vec3(texture(u_shadowmap, shadowmap_proj.xy).r), 1);
    //return;
    bool shadow = false;
    if (0.01 <= shadowmap_proj.x && 0.01 <= shadowmap_proj.y && shadowmap_proj.x <= 0.99
        && shadowmap_proj.y <= 0.99 &&
        shadowmap_proj.z > 0.000001 + texture(u_shadowmap, shadowmap_proj.xy).r) {
        shadow = true;
    }
        
    if (coordinates.y <= u_water_level)
        o_frag_color = common_main(u_water_color, u_light_wat, shadow);
    else
        o_frag_color = common_main(u_color, u_light, shadow);
}
