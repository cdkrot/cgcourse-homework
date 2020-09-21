#version 330 core

out vec4 o_frag_color;
in vec2 coordinates;

vec2 cmult(vec2 a, vec2 b) {
    return vec2(a.x * b.x - a.y * b.y, a.x * b.y + a.y * b.x);
}

uniform vec2 u_cvec;
uniform float u_R;
uniform int u_num_it;

uniform sampler2D grad;

// vec4 color_in = vec4(1, 0.3, 0.3, 1.0);
vec4 color_out = vec4(0, 0, 0, 1.0);

vec2 f_c(vec2 z) {
    return cmult(z, z) + u_cvec;
}

void main()
{
    // return texture(grad, 0.5 * (coordinates + vec2(1, 0)));
    vec2 cur = f_c(coordinates);

    for (int i = 1; i <= u_num_it; ++i, cur = f_c(cur))
        if (cur.x * cur.x + cur.y * cur.y <= u_R * u_R) {
            o_frag_color = texture(grad, vec2((i - 1) / float(u_num_it - 1), 0));
            return;
        }
    
    o_frag_color = color_out;
}
