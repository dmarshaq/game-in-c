#include "game/draw.h"
#include "game/graphics.h"
#include "core/core.h"
#include "core/type.h"
#include "core/mathf.h"


static const float DOT_SCALE = 0.005f;

void draw_quad(Vec2f p0, Vec2f p1, Vec4f color, Texture *texture, Vec2f uv0, Vec2f uv1, Texture *mask, float offset_angle, Vertex_Buffer *buffer) {
    float texture_slot = -1.0f; // @Important: -1.0f slot signifies shader to use color, not texture.
    float mask_slot = -1.0f;

    if (texture != NULL)
        texture_slot = add_texture_to_slots(texture);              

    if (mask != NULL)
        mask_slot = add_texture_to_slots(mask);              
    
    Vec2f k = vec2f_make(cosf(offset_angle), sinf(offset_angle));
    k = vec2f_multi_constant(k, vec2f_dot(k, vec2f_difference(p1, p0)));
    
    Vec2f p2 = vec2f_sum(p0, k);
    Vec2f p3 = vec2f_difference(p1, k);
    
    float quad_data[44] = {
        p0.x, p0.y, 0.0f, color.x, color.y, color.z, color.w, uv0.x, uv0.y, texture_slot, mask_slot,
        p2.x, p2.y, 0.0f, color.x, color.y, color.z, color.w, uv1.x, uv0.y, texture_slot, mask_slot,
        p3.x, p3.y, 0.0f, color.x, color.y, color.z, color.w, uv0.x, uv1.y, texture_slot, mask_slot,
        p1.x, p1.y, 0.0f, color.x, color.y, color.z, color.w, uv1.x, uv1.y, texture_slot, mask_slot,
    };
    
    if (buffer == NULL)
        draw_quad_data(quad_data, 1);
    else
        vertex_buffer_append_data(buffer, quad_data, VERTICIES_PER_QUAD * 11);
}

void draw_grid(Vec2f p0, Vec2f p1, Vec4f color, Vertex_Buffer *buffer) {
    Vec2f p2 = vec2f_make(p1.x, p0.y);
    Vec2f p3 = vec2f_make(p0.x, p1.y);
    
    float quad_data[36] = {
        p0.x, p0.y, 0.0f, color.x, color.y, color.z, color.w, p0.x, p0.y,
        p2.x, p2.y, 0.0f, color.x, color.y, color.z, color.w, p2.x, p2.y,
        p3.x, p3.y, 0.0f, color.x, color.y, color.z, color.w, p3.x, p3.y,
        p1.x, p1.y, 0.0f, color.x, color.y, color.z, color.w, p1.x, p1.y,
    };
    
    if (buffer == NULL)
        draw_quad_data(quad_data, 1);
    else
        vertex_buffer_append_data(buffer, quad_data, VERTICIES_PER_QUAD * 9);
}

void draw_text(String text, Vec2f current_point, Vec4f color, Font_Baked *font, u32 unit_scale, Vertex_Buffer *buffer) {
    // Scale and adjust current_point.
    current_point = vec2f_multi_constant(current_point, (float)unit_scale);
    float origin_x = current_point.x;
    current_point.y += (float)font->baseline;

    // Text rendering variables.
    s32 font_char_index;
    u16 width, height;
    stbtt_bakedchar *c;

    for (s64 i = 0; i < text.length; i++) {
        // @Incomplete: Handle special characters / symbols.
        if (text.data[i] == '\n') {
            current_point.x = origin_x;
            current_point.y -= (float)font->line_height;
            continue;
        }

        // Character drawing.
        font_char_index = (s32)text.data[i] - font->first_char_code;
        if (font_char_index < font->chars_count) {
            c = &font->chars[font_char_index];
            width  = font->chars[font_char_index].x1 - font->chars[font_char_index].x0;
            height = font->chars[font_char_index].y1 - font->chars[font_char_index].y0;

            draw_quad(vec2f_divide_constant(vec2f_make(current_point.x + c->xoff, current_point.y - c->yoff - height), (float)unit_scale), vec2f_divide_constant(vec2f_make(current_point.x + c->xoff + width, current_point.y - c->yoff), (float)unit_scale), color, NULL, vec2f_make(c->x0 / (float)font->bitmap.width, c->y1 / (float)font->bitmap.height), vec2f_make(c->x1 / (float)font->bitmap.width, c->y0 / (float)font->bitmap.height), &font->bitmap, 0.0f, buffer);

            current_point.x += font->chars[font_char_index].xadvance;
        }
    }
}

Vec2f text_size(String text, Font_Baked *font) {
    // Result.
    Vec2f result = VEC2F_ORIGIN;

    // Scale and adjust current_point.
    Vec2f current_point = VEC2F_ORIGIN;
    float origin_x = current_point.x;
    current_point.y += (float)font->baseline;

    // Text rendering variables.
    s32 font_char_index;

    for (s64 i = 0; i < text.length; i++) {
        // Handle special characters / symbols.
        if (text.data[i] == '\n') {
            result.y += (float)font->line_height;
            if (result.x < current_point.x)
                result.x = current_point.x;

            current_point.x = origin_x;
            current_point.y -= (float)font->line_height;
            continue;
        }

        // Character iterating.
        font_char_index = (s32)text.data[i] - font->first_char_code;
        if (font_char_index < font->chars_count) {

            current_point.x += font->chars[font_char_index].xadvance;
        }
    }
    
    // Since last line dimensions is not handled in the loop, it can be done here.
    if (text.data[text.length - 1] != '\n') 
        result.y += (float)font->line_height;

    if (result.x < current_point.x)
        result.x = current_point.x;

    return result;
}

float text_size_y(String text, Font_Baked *font) {
    // Result.
    float result = 0;

    // Text rendering variables.
    s32 font_char_index;

    for (s64 i = 0; i < text.length; i++) {
        // Handle special characters / symbols.
        if (text.data[i] == '\n') {
            result += (float)font->line_height;
            continue;
        }
    }
    
    // Since last line dimensions is not handled in the loop, it can be done here.
    if (text.data[text.length - 1] != '\n') 
        result += (float)font->line_height;

    return result;
}



void draw_line(Vec2f p0, Vec2f p1, Vec4f color, Vertex_Buffer *buffer) {
    float line_data[14] = {
        p0.x, p0.y, 0.0f, color.x, color.y, color.z, color.w,
        p1.x, p1.y, 0.0f, color.x, color.y, color.z, color.w,
    };

    if (buffer == NULL)
        draw_line_data(line_data, 1);
    else
        vertex_buffer_append_data(buffer, line_data, VERTICIES_PER_LINE * 7);
}


void draw_dot(Vec2f position, Vec4f color, Camera *camera, Vertex_Buffer *buffer) {
    draw_quad(vec2f_make(position.x - (float)camera->unit_scale * DOT_SCALE, position.y), vec2f_make(position.x + (float)camera->unit_scale * DOT_SCALE, position.y), color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, PI/4, buffer);
}

void draw_quad_outline(Vec2f p0, Vec2f p1, Vec4f color, float offset_angle, Vertex_Buffer *buffer) {
    
    Vec2f k = vec2f_make(cosf(offset_angle), sinf(offset_angle));
    k = vec2f_multi_constant(k, vec2f_dot(k, vec2f_difference(p1, p0)));
    
    Vec2f p2 = vec2f_sum(p0, k);
    Vec2f p3 = vec2f_difference(p1, k);
    
    float line_data[56] = {
        p0.x, p0.y, 0.0f, color.x, color.y, color.z, color.w,
        p2.x, p2.y, 0.0f, color.x, color.y, color.z, color.w,
        p2.x, p2.y, 0.0f, color.x, color.y, color.z, color.w,
        p1.x, p1.y, 0.0f, color.x, color.y, color.z, color.w,
        p1.x, p1.y, 0.0f, color.x, color.y, color.z, color.w,
        p3.x, p3.y, 0.0f, color.x, color.y, color.z, color.w,
        p3.x, p3.y, 0.0f, color.x, color.y, color.z, color.w,
        p0.x, p0.y, 0.0f, color.x, color.y, color.z, color.w,
    };

    if (buffer == NULL)
        draw_line_data(line_data, 4);
    else
        vertex_buffer_append_data(buffer, line_data, 4 * VERTICIES_PER_LINE * 7);
}

void draw_circle_outline(Vec2f position, float radius, u32 detail, Vec4f color, Vertex_Buffer *buffer) {

    float line_data[detail * 14];

    float step = 2*PI / (float)detail;
    for (u32 i = 0; i < detail; i++) {
        line_data[0  + i * 14] = radius * cosf(step * (float)i) + position.x;
        line_data[1  + i * 14] = radius * sinf(step * (float)i) + position.y;
        line_data[2  + i * 14] = 0.0f;
        line_data[3  + i * 14] = color.x;
        line_data[4  + i * 14] = color.y;
        line_data[5  + i * 14] = color.z;
        line_data[6  + i * 14] = color.w;
                     
        line_data[7  + i * 14] = radius * cosf(step * (float)(i + 1)) + position.x;
        line_data[8  + i * 14] = radius * sinf(step * (float)(i + 1)) + position.y;
        line_data[9  + i * 14] = 0.0f;
        line_data[10 + i * 14] = color.x;
        line_data[11 + i * 14] = color.y;
        line_data[12 + i * 14] = color.z;
        line_data[13 + i * 14] = color.w;
    }
    
   
    if (buffer == NULL)
        draw_line_data(line_data, detail);
    else
        vertex_buffer_append_data(buffer, line_data, detail * VERTICIES_PER_LINE * 7);
}

void draw_function(float x0, float x1, Function y, u32 detail, Vec4f color, Vertex_Buffer *buffer) {
    float line_data[detail * 14];

    float step = (x1 - x0) / (float)detail;
    for (u32 i = 0; i < detail; i++) {
        line_data[0  + i * 14] = x0 + step * (float)i;
        line_data[1  + i * 14] = y(x0 + step * (float)i);
        line_data[2  + i * 14] = 0.0f;
        line_data[3  + i * 14] = color.x;
        line_data[4  + i * 14] = color.y;
        line_data[5  + i * 14] = color.z;
        line_data[6  + i * 14] = color.w;
                     
        line_data[7  + i * 14] = x0 + step * (float)(i + 1);
        line_data[8  + i * 14] = y(x0 + step * (float)(i + 1));
        line_data[9  + i * 14] = 0.0f;
        line_data[10 + i * 14] = color.x;
        line_data[11 + i * 14] = color.y;
        line_data[12 + i * 14] = color.z;
        line_data[13 + i * 14] = color.w;
    }
    
   
    if (buffer == NULL)
        draw_line_data(line_data, detail);
    else
        vertex_buffer_append_data(buffer, line_data, detail * VERTICIES_PER_LINE * 7);
}

void draw_polar(float t0, float t1, Function r, u32 detail, Vec4f color, Vertex_Buffer *buffer) {
    float line_data[detail * 14];

    float step = (t1 - t0) / (float)detail;
    float radius;
    for (u32 i = 0; i < detail; i++) {
        radius = r(t0 + step * (float)i);
        line_data[0  + i * 14] = radius * cosf(t0 + step * (float)i);
        line_data[1  + i * 14] = radius * sinf(t0 + step * (float)i);
        line_data[2  + i * 14] = 0.0f;
        line_data[3  + i * 14] = color.x;
        line_data[4  + i * 14] = color.y;
        line_data[5  + i * 14] = color.z;
        line_data[6  + i * 14] = color.w;
                     
        radius = r(t0 + step * (float)(i + 1));
        line_data[7  + i * 14] = radius * cosf(t0 + step * (float)(i + 1));
        line_data[8  + i * 14] = radius * sinf(t0 + step * (float)(i + 1));
        line_data[9  + i * 14] = 0.0f;
        line_data[10 + i * 14] = color.x;
        line_data[11 + i * 14] = color.y;
        line_data[12 + i * 14] = color.z;
        line_data[13 + i * 14] = color.w;
    }
    
   
    if (buffer == NULL)
        draw_line_data(line_data, detail);
    else
        vertex_buffer_append_data(buffer, line_data, detail * VERTICIES_PER_LINE * 7);
}

void draw_parametric(float t0, float t1, Function x, Function y, u32 detail, Vec4f color, Vertex_Buffer *buffer) {
    float line_data[detail * 14];

    float step = (t1 - t0) / (float)detail;
    for (u32 i = 0; i < detail; i++) {
        line_data[0  + i * 14] = x(t0 + step * (float)i);
        line_data[1  + i * 14] = y(t0 + step * (float)i);
        line_data[2  + i * 14] = 0.0f;
        line_data[3  + i * 14] = color.x;
        line_data[4  + i * 14] = color.y;
        line_data[5  + i * 14] = color.z;
        line_data[6  + i * 14] = color.w;

        line_data[7  + i * 14] = x(t0 + step * (float)(i + 1));
        line_data[8  + i * 14] = y(t0 + step * (float)(i + 1));
        line_data[9  + i * 14] = 0.0f;
        line_data[10 + i * 14] = color.x;
        line_data[11 + i * 14] = color.y;
        line_data[12 + i * 14] = color.z;
        line_data[13 + i * 14] = color.w;
    }
    
   
    if (buffer == NULL)
        draw_line_data(line_data, detail);
    else
        vertex_buffer_append_data(buffer, line_data, detail * VERTICIES_PER_LINE * 7);
}

void draw_area_function(float x0, float x1, Function y, u32 rect_count, Vec4f color, Vertex_Buffer *buffer) {
    float step = (x1 - x0) / (float)rect_count;
    for (u32 i = 0; i < rect_count; i++) {
        draw_quad(vec2f_make(x0 + step * i, 0.0f), vec2f_make(x0 + step * (i + 1), y(x0 + step * (i + 1))), color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, 0.0f, buffer);
    }
}

void draw_area_polar(float t0, float t1, Function r, u32 rect_count, Vec4f color, Vertex_Buffer *buffer) {
    float quad_data[rect_count * VERTICIES_PER_QUAD * 11];

    float step = (t1 - t0) / (float)rect_count;
    float radius;
    for (u32 i = 0; i < rect_count; i++) {
        quad_data[0  + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[1  + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[2  + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[3  + i * VERTICIES_PER_QUAD * 11] = color.x;
        quad_data[4  + i * VERTICIES_PER_QUAD * 11] = color.y;
        quad_data[5  + i * VERTICIES_PER_QUAD * 11] = color.z;
        quad_data[6  + i * VERTICIES_PER_QUAD * 11] = color.w;
        quad_data[7  + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[8  + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[9  + i * VERTICIES_PER_QUAD * 11] = -1.0f;
        quad_data[10 + i * VERTICIES_PER_QUAD * 11] = -1.0f;
                     
        quad_data[11 + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[12 + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[13 + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[14 + i * VERTICIES_PER_QUAD * 11] = color.x;
        quad_data[15 + i * VERTICIES_PER_QUAD * 11] = color.y;
        quad_data[16 + i * VERTICIES_PER_QUAD * 11] = color.z;
        quad_data[17 + i * VERTICIES_PER_QUAD * 11] = color.w;
        quad_data[18 + i * VERTICIES_PER_QUAD * 11] = 1.0f;
        quad_data[19 + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[20 + i * VERTICIES_PER_QUAD * 11] = -1.0f;
        quad_data[21 + i * VERTICIES_PER_QUAD * 11] = -1.0f;
                     
        radius = r(t0 + step * (float)i);
        quad_data[22 + i * VERTICIES_PER_QUAD * 11] = radius * cosf(t0 + step * (float)i);
        quad_data[23 + i * VERTICIES_PER_QUAD * 11] = radius * sinf(t0 + step * (float)i);
        quad_data[24 + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[25 + i * VERTICIES_PER_QUAD * 11] = color.x;
        quad_data[26 + i * VERTICIES_PER_QUAD * 11] = color.y;
        quad_data[27 + i * VERTICIES_PER_QUAD * 11] = color.z;
        quad_data[28 + i * VERTICIES_PER_QUAD * 11] = color.w;
        quad_data[29 + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[30 + i * VERTICIES_PER_QUAD * 11] = 1.0f;
        quad_data[31 + i * VERTICIES_PER_QUAD * 11] = -1.0f;
        quad_data[32 + i * VERTICIES_PER_QUAD * 11] = -1.0f;
                     
        radius = r(t0 + step * (float)(i + 1));
        quad_data[33 + i * VERTICIES_PER_QUAD * 11] = radius * cosf(t0 + step * (float)(i + 1));
        quad_data[34 + i * VERTICIES_PER_QUAD * 11] = radius * sinf(t0 + step * (float)(i + 1));
        quad_data[35 + i * VERTICIES_PER_QUAD * 11] = 0.0f;
        quad_data[36 + i * VERTICIES_PER_QUAD * 11] = color.x;
        quad_data[37 + i * VERTICIES_PER_QUAD * 11] = color.y;
        quad_data[38 + i * VERTICIES_PER_QUAD * 11] = color.z;
        quad_data[39 + i * VERTICIES_PER_QUAD * 11] = color.w;
        quad_data[40 + i * VERTICIES_PER_QUAD * 11] = 1.0f;
        quad_data[41 + i * VERTICIES_PER_QUAD * 11] = 1.0f;
        quad_data[42 + i * VERTICIES_PER_QUAD * 11] = -1.0f;
        quad_data[43 + i * VERTICIES_PER_QUAD * 11] = -1.0f;
    }
    
    if (buffer == NULL)
        draw_quad_data(quad_data, rect_count);
    else
        vertex_buffer_append_data(buffer, quad_data, rect_count * VERTICIES_PER_QUAD * 11);

}

void draw_area_parametric(float t0, float t1, Function x, Function y, u32 rect_count, Vec4f color, Vertex_Buffer *buffer) {
    float step = (t1 - t0) / (float)rect_count;
    for (u32 i = 0; i < rect_count; i++) {
        draw_quad(vec2f_make(x(t0 + step * i), 0.0f), vec2f_make(x(t0 + step * (i + 1)), y(t0 + step * (i + 1))), color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, 0.0f, buffer);
    }
}


void draw_viewport(u32 x, u32 y, u32 width, u32 height, Vec4f color, Camera *camera, Vertex_Buffer *buffer) {
    glViewport(x, y, width, height);
    
    Vec2f p1 = vec2f_make((float)width / 2 / (float)camera->unit_scale, (float)height / 2 / (float)camera->unit_scale);
    Vec2f p0 = vec2f_negate(p1);
    
    draw_quad(p0, p1, color, NULL, p0, p1, NULL, 0.0f, buffer);

}

void viewport_reset(float window_width, float window_height) {
    glViewport(0, 0, window_width, window_height);
}

Vec2f camera_screen_to_world(Vec2f point, Camera *camera, float window_width, float window_height) {
    return vec2f_make(camera->center.x + (point.x - window_width / 2) / (float)camera->unit_scale, camera->center.y + (point.y - window_height / 2) / (float)camera->unit_scale);
}
