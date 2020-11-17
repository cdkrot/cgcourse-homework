#version 330 core

in vec3 coordinates;
in vec3 normal_;
in vec3 vs_color;
out vec4 o_frag_color;

uniform vec3 u_camera;
uniform vec3 u_sun_direction;
uniform vec4 u_color;
uniform vec3 u_light;
uniform mat4 u_lightmat;
uniform sampler2D u_shadowmap;

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

void main() {
    vec3 normal = normalize(normal_);
    vec3 to_camera = normalize(u_camera - coordinates);
    vec4 shadowmap_proj_ = u_lightmat * vec4(coordinates, 1);
    vec3 shadowmap_proj = shadowmap_proj_.xyz / shadowmap_proj_.w;
    shadowmap_proj = 0.5 * (shadowmap_proj + vec3(1,1,1));

    //o_frag_color = vec4(vec3(texture(u_shadowmap, shadowmap_proj.xy).r), 1);
    //return;
    bool shadow = false;
    if (0 <= shadowmap_proj.x && 0 <= shadowmap_proj.y && shadowmap_proj.x <= 1
        && shadowmap_proj.y <= 1 &&
        shadowmap_proj.z > texture(u_shadowmap, shadowmap_proj.xy).r + 0.03) {
        shadow = true;
    }
    
    float light_factor = dot(u_light,
                             get_shininess(normal, u_sun_direction, to_camera, shadow));
    o_frag_color = vec4(u_color.xyz * light_factor, u_color.z);
}
