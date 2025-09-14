#ifndef DRAW_H
#define DRAW_H

#include "game/graphics.h"

#include "core/core.h"
#include "core/type.h"
#include "core/mathf.h"
#include "core/str.h"



typedef struct draw_quad_args_opt {
    Vec4f           color;
    Texture *       texture;
    Vec2f           uv0;
    Vec2f           uv1;
    Texture *       mask;
    Vertex_Buffer * buffer;
} Draw_Quad_Opt_Args;

#define draw_quad(p0, p2, p3, p1, ...)  draw_quad_opt(p0, p2, p3, p1, (Draw_Quad_Opt_Args) { .color = VEC4F_PINK, .texture = NULL, .uv0 = VEC2F_ORIGIN, .uv1 = VEC2F_UNIT, .mask = NULL, .buffer = NULL, __VA_ARGS__})

/**
 * Draws p0 being bottom left corner, p1 being top right corner.
 */
void draw_quad_opt(Vec2f p0, Vec2f p2, Vec2f p3, Vec2f p1, Draw_Quad_Opt_Args opt);








typedef struct draw_rect_args_opt {
    Vec4f           color;
    Texture *       texture;
    Vec2f           uv0;
    Vec2f           uv1;
    Texture *       mask;
    float           offset_angle;
    Vertex_Buffer * buffer;
} Draw_Rect_Opt_Args;

#define draw_rect(p0, p1, ...)  draw_rect_opt(p0, p1, (Draw_Rect_Opt_Args) { .color = VEC4F_PINK, .texture = NULL, .uv0 = VEC2F_ORIGIN, .uv1 = VEC2F_UNIT, .mask = NULL, .offset_angle = 0.0f, .buffer = NULL, __VA_ARGS__})

/**
 * Draws rectangular quad, p0 being bottom left corner, p1 being top right corner.
 */
void draw_rect_opt(Vec2f p0, Vec2f p1, Draw_Rect_Opt_Args opt);





typedef struct draw_text_args_opt {
    Vec4f           color;
    u32             unit_scale;
    Vertex_Buffer * buffer;
} Draw_Text_Opt_Args;

#define draw_text(text, position, font, ...)  draw_text_opt(text, position, font, (Draw_Text_Opt_Args) { .color = VEC4F_WHITE, .unit_scale = 1, .buffer = NULL, __VA_ARGS__})

/**
 * Draws text as series of quads, position being the top left origin of the text.
 */
void draw_text_opt(String text, Vec2f position, Font_Baked *font, Draw_Text_Opt_Args opt);

/**
 * Returns width and height of the text.
 */
Vec2f text_size(String text, Font_Baked *font);

/**
 * Returns width the text.
 */
float text_size_y(String text, Font_Baked *font);







// @Todo: Write proper comments for each draw function, and properly supply optionals there, if there are any.


// Drawing lines / dots.
void draw_line(Vec2f p0, Vec2f p1, Vec4f color, Vertex_Buffer *buffer);
void draw_dot(Vec2f position, Vec4f color, Camera *camera, Vertex_Buffer *buffer);
void draw_cross(Vec2f position, Vec4f color, Camera *camera, Vertex_Buffer *buffer);

// Drawing frames.
void draw_quad_outline(Vec2f p0, Vec2f p2, Vec2f p3, Vec2f p1, Vec4f color, Vertex_Buffer *buffer);
void draw_rect_outline(Vec2f p0, Vec2f p1, Vec4f color, float offset_angle, Vertex_Buffer *buffer);
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
