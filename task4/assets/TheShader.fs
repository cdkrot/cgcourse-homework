#version 330 core

in vec3 screen_position;
uniform vec3 u_camera;
uniform mat4 u_vp_inv;
out vec3 o_frag_color;

const int trace_steps = 190; // 300;
const float eps = 1e-3;
const float trace_max_dist = 1000;

const float ambient_light = 0.4;
const float diff_light = 0.6;
const float spec_light = 0.4;

struct Object {
    vec3 offset;
    float scale;
    int sdf_type;
    int colorf_type;
    vec3 color_info;
};

struct Light {
    vec3 pos;
    vec3 color;
    
    int linked_object; // or -1
};

#define SDF_SPHERE 0
#define SDF_PLANE 1
#define SDF_CUBE 2
#define SDF_XCUBE 3
#define COLORF_CONST 0
#define COLORF_MIRROR 1

float sdf_sphere(vec3 pt) {
    return length(pt) - 1;
}

float sdf_halfplane(vec3 pt) {
    return max(float(0.0), pt.y);
}

// [-0.5, +0.5]^3
float sdf_cube(vec3 pt) {
    if (-0.5 <= min(pt.x, min(pt.y, pt.z)) && max(pt.x, max(pt.y, pt.z)) <= 0.5) // inside
        return min(abs(pt.x), min(abs(pt.y), abs(pt.z))) - 0.5;

    return max(abs(pt.x), max(abs(pt.y), abs(pt.z))) - 0.5;
}

float sdf_xcube(vec3 pt) {
    float res = max(sdf_cube(pt), sdf_sphere(pt / 0.7) * 0.7);

    res = max(res, -sdf_sphere((pt - vec3(0.5,0,0)) / 0.3) * 0.3);
    res = max(res, -sdf_sphere((pt + vec3(0.5,0,0)) / 0.3) * 0.3);
    
    res = max(res, -sdf_sphere((pt - vec3(0,0.5,0)) / 0.3) * 0.3);
    res = max(res, -sdf_sphere((pt + vec3(0,0.5,0)) / 0.3) * 0.3);
    res = max(res, -sdf_sphere((pt - vec3(0,0.5,0)) / 0.3) * 0.3);
    res = max(res, -sdf_sphere((pt + vec3(0,0.5,0)) / 0.3) * 0.3);
    return res;
}

float sdf_object(Object obj, vec3 pt) {
    pt = pt - obj.offset;

    switch (obj.sdf_type) {
    case SDF_SPHERE:
        return sdf_sphere(pt / obj.scale) * obj.scale;
    case SDF_PLANE:
        return sdf_halfplane(pt / obj.scale) * obj.scale;
    case SDF_CUBE:
        return sdf_cube(pt / obj.scale) * obj.scale;
    case SDF_XCUBE:
        return sdf_xcube(pt / obj.scale) * obj.scale;
    }

    return -1.0f;
}

const Object sphere0 = Object(vec3(0,0,0), 1, SDF_SPHERE, COLORF_CONST, vec3(0.3, 0, 0));
const Object sphere1 = Object(vec3(0,10,0), 4, SDF_SPHERE, COLORF_CONST, vec3(0.0, 0.3, 0));
const Object sphere2 = Object(vec3(0,0,15), 2, SDF_SPHERE, COLORF_CONST, vec3(0, 0, 0.3));
const Object cube   = Object(vec3(-3,0,12), 1, SDF_CUBE, COLORF_CONST, vec3(0.1, 0.1, 0.1));
const Object cube2  = Object(vec3(-27,2,9),  9, SDF_XCUBE, COLORF_CONST, vec3(0.2, 0.2, 0.2));
const Object mirrorcube = Object(vec3(-4,5,-91), 40, SDF_CUBE, COLORF_MIRROR, vec3(0.2,0.2,0.2));
const Object ground = Object(vec3(0,-5,0), 1, SDF_PLANE, COLORF_CONST, vec3(0.1, 0.5, 0.2));
const Object light_sphere = Object(vec3(-10, 20, 0), 0.2, SDF_SPHERE, COLORF_CONST, vec3(1,1,1));

const Object obj_list[8] = Object[8](ground, mirrorcube,
                                     light_sphere, sphere0, sphere1, sphere2, cube, cube2);

const Light lights[1] = Light[1](Light(vec3(-10, 20, 0), vec3(1.0, 0.5, 0.6), 2));
//                                 Light(vec3(21, 8, 52), vec3(1.0, 0.0, 0.0), -1));
                             
float sceneSDF(vec3 pt, out int obj_ind) {
    float dist = 1e9;
    obj_ind = -1;

    // ignore ground here for performance reasons
    for (int i = 1; i < obj_list.length(); ++i) {
        float tmp = sdf_object(obj_list[i], pt);

        if (tmp < dist)
            dist = tmp, obj_ind = i;
    }
    
    return dist;
}
float sceneSDF(vec3 pt) {
    int ig;
    return sceneSDF(pt, ig);
}

vec3 estimateNormal(vec3 p) {
    if (abs(p.y - ground.offset.y) <= eps)
        return vec3(0,1,0);
    
    return normalize(vec3(
        sceneSDF(vec3(p.x + eps, p.y, p.z)) - sceneSDF(vec3(p.x - eps, p.y, p.z)),
        sceneSDF(vec3(p.x, p.y + eps, p.z)) - sceneSDF(vec3(p.x, p.y - eps, p.z)),
        sceneSDF(vec3(p.x, p.y, p.z  + eps)) - sceneSDF(vec3(p.x, p.y, p.z - eps))
    ));
}

int trace(vec3 start, vec3 direction, int steps, out vec3 cur) {
    cur = start;
    float total_len = 0;
    
    for (int i = 0; i < steps; ++i) {
        int obj_index;
        float sdf = sceneSDF(cur, obj_index);

        if (direction.y < -eps) { // handle ground
            float tmp = (ground.offset.y - cur.y) / direction.y;
            if (tmp < sdf)
                sdf = tmp, obj_index = 0;
        }
        
        if (sdf <= eps) {
            cur += sdf * direction;
            return obj_index; // hit
        }

        cur += direction * sdf;
        total_len += sdf;

        if (total_len >= trace_max_dist)
            return -1;
    }
    
    return -2; // whoops, haven't converged :/
}

int trace(vec3 start, vec3 direction, int steps) {
    vec3 cur;
    return trace(start, direction, steps, cur);
}

vec3 colorf_const(Object obj) {
    return obj.color_info;
}

vec3 colorf_mirror(Object obj, vec3 pos, vec3 normal, vec3 viewdir) {
    pos += 0.1 * normal;

    vec3 hitpos;
    int obj_no = trace(pos, reflect(viewdir, normal), trace_steps, hitpos);
    
    vec3 ray_result;
    if (obj_no == -2)
        ray_result = vec3(0);
    else if (obj_no == -1)
        ray_result = vec3(0.8,0.8,1.0);
    else
        ray_result = colorf_const(obj_list[obj_no]);
    
    return 0.3 * obj.color_info + 0.7 * ray_result; 
}

vec3 colorf_object(Object obj, vec3 pos, vec3 normal, vec3 viewdir) {
    switch (obj.colorf_type) {
    case COLORF_CONST:
        return colorf_const(obj);
    case COLORF_MIRROR:
        return colorf_mirror(obj, pos, normal, viewdir);
    }

    return vec3(-1,-1,-1);
}

void main() {
    vec4 tmp = u_vp_inv * vec4(screen_position, 1.0);
    vec3 direction = normalize((tmp.xyz/tmp.w) - u_camera);

    vec3 hitpos;
    int obj = trace(u_camera, direction, trace_steps, hitpos);
    vec3 normal = estimateNormal(hitpos);
    hitpos += normal * 0.01;
    
    if (obj == -2) { // oopsie-doopsie
        o_frag_color = vec3(0.0,0.0,0.0);
    } else if (obj == -1)
        o_frag_color = vec3(0.8,0.8,1.0);
    else {
        vec3 color = colorf_object(obj_list[obj], hitpos, normal, direction);
        bool is_light = false;
        for (int i = 0; i < lights.length(); ++i)
            if (lights[i].linked_object == obj)
                is_light = true;

        if (is_light) {
            o_frag_color = color;
            return;
        }
        
        vec3 brightness = vec3(ambient_light);

        for (int i = 0; i < lights.length(); ++i) {
            vec3 light_direction = normalize(lights[i].pos - hitpos);
            vec3 refl_dir = reflect(-light_direction, normal);
            
            if (trace(hitpos, light_direction, trace_steps) == lights[i].linked_object) {
                brightness += diff_light * lights[i].color * max(0.f, dot(normal, light_direction));
                brightness += spec_light * lights[i].color *
                    pow(max(dot(direction, -refl_dir), 0.0), 20);
            }
;
        }
        
        o_frag_color = brightness * color;
    }
}
