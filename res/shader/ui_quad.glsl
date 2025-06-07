#version 430 core

#ifdef VERTEX

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv0;
layout(location = 3) in float aspect;
layout(location = 4) in float mask_index;

layout(location = 0) uniform mat4 pr_matrix;
layout(location = 4) uniform mat4 ml_matrix;

out vec4 v_color;
out vec2 v_uv0;
out float v_mask_index;
out float v_aspect;

void main() {
    v_color = color;
    v_uv0 = uv0;
    v_aspect = aspect;
    v_mask_index = mask_index;

    gl_Position = pr_matrix * ml_matrix * vec4(position, 1.0);
}

#endif

#ifdef FRAGMENT


layout(location = 0) out vec4 color;
layout(location = 8) uniform sampler2D u_textures[32];

in vec4 v_color;
in vec2 v_uv0;
in float v_aspect;
in float v_mask_index;

#define OFFSET 10

void main() {
    int mask_index = int(v_mask_index);

    // Temporary solution for border and rounded corners, until better shader loading.
    if (mask_index == -1) {
        vec2 uv = (v_uv0 - 0.5) * 2 * OFFSET;
        float d;

        if (v_aspect < 1.0) {
            uv.y = uv.y / v_aspect;
            d = distance(vec2(clamp(uv.x, -OFFSET + 1, OFFSET - 1), clamp(uv.y, -OFFSET / v_aspect + 1, OFFSET / v_aspect - 1)), uv);
        } else {
            uv.x = uv.x * v_aspect;
            d = distance(vec2(clamp(uv.x, -OFFSET * v_aspect + 1, OFFSET * v_aspect - 1), clamp(uv.y, -OFFSET + 1, OFFSET - 1)), uv);
        }

        
        float border = step(1.0, d);
        d = step(d, 0.2);

        color = vec4((1 - border.xxx) * v_color.xyz + d.xxx * v_color.xyz, 1 - border);

    }
    else {
        color = vec4(v_color.xyz, v_color.w * texture(u_textures[mask_index], v_uv0).x);
    }
}

#endif
