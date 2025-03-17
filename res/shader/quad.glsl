#version 430 core

#ifdef VERTEX

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv0;
layout(location = 3) in float tex_index;
layout(location = 4) in float mask_index;

layout(location = 0) uniform mat4 pr_matrix;
layout(location = 4) uniform mat4 ml_matrix;

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

layout(location = 0) out vec4 color;

in vec4 v_color;
in vec2 v_uv0;
in float v_tex_index;
in float v_mask_index;

layout(location = 8) uniform sampler2D u_textures[32];

void main() {
    int tex_index = int(v_tex_index);
    int mask_index = int(v_mask_index);

    // Base color.
    if (tex_index == -1) {
        color = v_color;
    }
    else {
        color = texture(u_textures[tex_index], v_uv0);
    }

    // Applying mask.
    if (mask_index != -1) {
        color.w = texture(u_textures[mask_index], v_uv0).x * color.w;
    }
}

#endif
