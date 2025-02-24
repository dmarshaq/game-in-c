#version 330 core

#ifdef VERTEX

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv0;
layout(location = 3) in float tex_index;
layout(location = 4) in float mask_index;

uniform mat4 pr_matrix;
uniform mat4 ml_matrix;

out vec4 v_color;
out vec2 v_uv0;
out float v_tex_index;
out float v_mask_index;

void main() {
    v_color = color;
    v_uv0 = uv0;
    v_tex_index = tex_index;
    v_mask_index = mask_index;

    gl_Position = pr_matrix * ml_matrix * vec4(position, 1.0);
}

#endif

#ifdef FRAGMENT

#define PI 3.14159265359
#define AMP 16

layout(location = 0) out vec4 color;

in vec4 v_color;
in vec2 v_uv0;
in float v_tex_index;
in float v_mask_index;

uniform sampler2D u_textures[32];

float ground(float x) {
    return (abs(x) + x) / 2;
}

void main() {
    color.xyz = v_color.xyz * (ground(1 - abs(sin(v_uv0.x * PI) * AMP)) + ground(1 - abs(sin(v_uv0.y * PI) * AMP)));
    color.x += ground(0.15 - pow(abs(v_uv0.y), 0.5)) * 24;
    color.y += ground(0.15 - pow(abs(v_uv0.x), 0.5)) * 24;
    color.w = v_color.w;
}

#endif
