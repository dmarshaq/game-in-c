#version 330 core

#ifdef VERTEX

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 uv0;
layout(location = 3) in float tex_index;

uniform mat4 pr_matrix;
uniform mat4 ml_matrix;

out vec4 v_color;
out vec2 v_uv0;
out float v_tex_index;

void main() {
    v_color = color;
    v_uv0 = uv0;
    v_tex_index = tex_index;

    gl_Position = pr_matrix * ml_matrix * vec4(position, 1.0);
}

#endif

#ifdef FRAGMENT

layout(location = 0) out vec4 color;

in vec4 v_color;
in vec2 v_uv0;
in float v_tex_index;

uniform sampler2D u_textures[32];

void main() {
    int index = int(v_tex_index);
    if (index == -1) {
        color = v_color;
    }
    else {
        vec4 tex_color = texture(u_textures[index], v_uv0);
        color = tex_color;
    }
}

#endif
