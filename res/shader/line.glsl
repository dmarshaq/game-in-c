#version 430 core

#ifdef VERTEX

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

layout(location = 0) uniform mat4 pr_matrix;
layout(location = 4) uniform mat4 ml_matrix;

out vec4 v_color;

void main() {
    v_color = color;

    gl_Position = pr_matrix * ml_matrix * vec4(position, 1.0);
}

#endif

#ifdef FRAGMENT

layout(location = 0) out vec4 color;

in vec4 v_color;

layout(location = 8) uniform sampler2D u_textures[32];

void main() {
    color = v_color;
}

#endif
