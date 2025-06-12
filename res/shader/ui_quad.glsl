#version 430 core

#ifdef VERTEX

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv0;
layout(location = 3) in vec2 size;
layout(location = 4) in float mask_index;

layout(location = 0) uniform mat4 pr_matrix;
layout(location = 4) uniform mat4 ml_matrix;

out vec4 v_color;
out vec2 v_uv0;
out vec2 v_size;
out float v_mask_index;

void main() {
    v_color = color;
    v_uv0 = uv0;
    v_size = size;
    v_mask_index = mask_index;

    gl_Position = pr_matrix * ml_matrix * vec4(position, 1.0);
}

#endif

#ifdef FRAGMENT


layout(location = 0) out vec4 color;

layout(location = 8) uniform sampler2D u_textures[32];

in vec4 v_color;
in vec2 v_uv0;
in vec2 v_size;
in float v_mask_index;

#define ROUNDNESS 10.0
#define BORDER 2.0
#define AA 1.0

void main() {
    int mask_index = int(v_mask_index);

    if (mask_index == -1) {
        vec2 uv = (v_uv0 - 0.5) * 2;
        vec2 scale = vec2(v_size.x / ROUNDNESS * 0.5, v_size.y / ROUNDNESS * 0.5);
        uv.x *= scale.x;
        uv.y *= scale.y;

        float d = distance(vec2(clamp(uv.x, -scale.x + 1, scale.x - 1), clamp(uv.y, -scale.y + 1, scale.y - 1)), uv);

        float outer = 1.0 - smoothstep(1.0 - AA / ROUNDNESS, 1.0 + AA / ROUNDNESS, d);
        float alpha = outer * v_color.a;

        float fill = 1.0 - smoothstep(1.0 - (BORDER + AA) / ROUNDNESS, 1.0 - (BORDER - AA) / ROUNDNESS, d);
        vec3 fill_rgb = mix(vec3(0.0, 0.0, 0.0), v_color.xyz, fill);

        color = vec4(fill_rgb, alpha);
    } else {
        color = v_color;
        color.w = texture(u_textures[mask_index], v_uv0).x * color.w;
    }

}

#endif
