#ifndef DRAW_H
#define DRAW_H

#include "core/core.h"
#include "core/type.h"
#include "core/mathf.h"
#include "core/graphics.h"

// Drawing basics.
void draw_quad(Vec2f p0, Vec2f p1, Vec4f color, Texture *texture, Vec2f uv0, Vec2f uv1, Texture *mask, float offset_angle, Vertex_Buffer *buffer);
void draw_grid(Vec2f p0, Vec2f p1, Vec4f color, Vertex_Buffer *buffer);
void draw_text(const char *text, Vec2f position, Vec4f color, Font_Baked *font, u32 unit_scale, Vertex_Buffer *buffer);
Vec2f text_size(const char *text, Font_Baked *font);
void draw_line(Vec2f p0, Vec2f p1, Vec4f color, Vertex_Buffer *buffer);
void draw_dot(Vec2f position, Vec4f color, Camera *camera, Vertex_Buffer *buffer);

// Drawing frames.
void draw_quad_outline(Vec2f p0, Vec2f p1, Vec4f color, float offset_angle, Vertex_Buffer *buffer);
void draw_circle_outline(Vec2f position, float radius, u32 detail, Vec4f color, Vertex_Buffer *buffer);

// Drawing function.
void draw_function(float x0, float x1, Function y, u32 detail, Vec4f color, Vertex_Buffer *buffer);
void draw_polar(float t0, float t1, Function r, u32 detail, Vec4f color, Vertex_Buffer *buffer);
void draw_parametric(float t0, float t1, Function x, Function y, u32 detail, Vec4f color, Vertex_Buffer *buffer);

// Drawing area under the curve.
void draw_area_function(float x0, float x1, Function y, u32 rect_count, Vec4f color, Vertex_Buffer *buffer);
void draw_area_polar(float t0, float t1, Function r, u32 rect_count, Vec4f color, Vertex_Buffer *buffer);
void draw_area_parametric(float t0, float t1, Function x, Function y, u32 rect_count, Vec4f color, Vertex_Buffer *buffer);

// Viewport manipulation.
void draw_viewport(u32 x, u32 y, u32 width, u32 height, Vec4f color, Camera *camera, Vertex_Buffer *buffer);
void viewport_reset(float window_width, float window_height);

// 2D screen point translation to 2D world.
Vec2f camera_screen_to_world(Vec2f point, Camera *camera, float window_width, float window_height);

#endif
