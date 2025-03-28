#include "plug.h"
#include "SDL2/SDL_keycode.h"
#include "core.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

// Helper functions interface.
void draw_quad(Vec2f p0, Vec2f p1, Vec4f color, Texture *texture, Vec2f uv0, Vec2f uv1, Texture *mask, float offset_angle);
void draw_text(const char *text, Vec2f position, Vec4f color, Font_Baked *font, u32 unit_scale);
void draw_line(Vec2f p0, Vec2f p1, Vec4f color);

void draw_function(float x0, float x1, Function y, u32 detail, Vec4f color);
void draw_polar(float t0, float t1, Function r, u32 detail, Vec4f color);
void draw_parametric(float t0, float t1, Function x, Function y, u32 detail, Vec4f color);

void draw_area_function(float x0, float x1, Function y, u32 rect_count, Vec4f color);
void draw_area_polar(float t0, float t1, Function r, u32 rect_count, Vec4f color);
void draw_area_parametric(float t0, float t1, Function x, Function y, u32 rect_count, Vec4f color);

void draw_viewport(u32 x, u32 y, u32 width, u32 height, Vec4f color, Camera *camera);
void viewport_reset(Plug_State *state);



Plug_State *global_state;

const Vec2f gravity_force = vec2f_make(0.0f, -9.0f);
const float jump_input_leeway = 0.2f;



void plug_init(Plug_State *state) {
    /**
     * Setting hash tables for resources. 
     * Since they are dynamically allocated pointers will not change after hot reloading.
     */
    state->shader_table = hash_table_make(Shader, 8, &std_allocator);
    state->font_table = hash_table_make(Font_Baked, 8, &std_allocator);

    state->player = (Player) {
        .center = VEC2F_ORIGIN,
        .width = 1.0f,
        .height = 1.25f,
        .velocity = VEC2F_ORIGIN,
    };
}


void plug_load(Plug_State *state) {
    state->clear_color = vec4f_make(0.1f, 0.1f, 0.4f, 1.0f);

    // Main camera init.
    state->main_camera = camera_make(VEC2F_ORIGIN, 64);

    // Shader loading.
    Shader quad_shader = shader_load("res/shader/quad.glsl");
    quad_shader.vertex_stride = 11;
    hash_table_put(&state->shader_table, quad_shader, "quad", 4);
    
    Shader grid_shader = shader_load("res/shader/grid.glsl");
    grid_shader.vertex_stride = 11;
    shader_set_uniforms(&grid_shader);
    hash_table_put(&state->shader_table, grid_shader, "grid", 4);

    Shader line_shader = shader_load("res/shader/line.glsl");
    line_shader.vertex_stride = 7;
    hash_table_put(&state->shader_table, line_shader, "line", 4);

    // Drawer init.
    drawer_init(&state->drawer, &(state->shader_table[hash_table_get_index_of(&state->shader_table, "quad", 4)]));

    // Drawer init.
    line_drawer_init(&state->line_drawer, &(state->shader_table[hash_table_get_index_of(&state->shader_table, "line", 4)]));


    // Font loading.
    u8* font_data = read_file_into_buffer("res/font/font.ttf", NULL, &std_allocator);

    Font_Baked font_baked_medium = font_bake(font_data, 20.0f);
    hash_table_put(&state->font_table, font_baked_medium, "medium", 6);

    Font_Baked font_baked_small = font_bake(font_data, 16.0f);
    hash_table_put(&state->font_table, font_baked_small, "small", 5);

    free(font_data);

    // Player loading.
    state->player.speed = 8.0f;

    global_state = state;
}





/**
 * Interface for functions.
 */
void update_player(Player *p);

void draw_player(Player *p);



void plug_update(Plug_State *state) {
    // Get shaders and fonts.
    Shader *quad_shader = &(state->shader_table[hash_table_get_index_of(&state->shader_table, "quad", 4)]);
    Shader *grid_shader = &(state->shader_table[hash_table_get_index_of(&state->shader_table, "grid", 4)]);
    Shader *line_shader = &(state->shader_table[hash_table_get_index_of(&state->shader_table, "line", 4)]);

    Font_Baked *font_medium = &state->font_table[hash_table_get_index_of(&state->font_table, "medium", 6)];
    Font_Baked *font_small = &state->font_table[hash_table_get_index_of(&state->font_table, "small", 5)];

    // Reset viewport.
    viewport_reset(state);

    // Clear the screen with clear_color.
    glClearColor(state->clear_color.x, state->clear_color.y, state->clear_color.z, state->clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw grid background.
    shader_update_projection(grid_shader, &state->main_camera, (float)state->window_width, (float)state->window_height);




    state->drawer.program = grid_shader;
    draw_begin(&state->drawer);

    Vec2f p0, p1;
    p0 = vec2f_make(-8.0f, -5.0f);
    p1 = vec2f_make(8.0f, 5.0f);
    draw_quad(p0, p1, vec4f_make(0.8f, 0.8f, 0.8f, 0.6f), NULL, p0, p1, NULL, 0.0f);

    draw_end();

    // Updating projection, regular quad drawing.
    state->drawer.program = quad_shader;
    shader_update_projection(quad_shader, &state->main_camera, (float)state->window_width, (float)state->window_height);
    
    // Player logic.
    update_player(&state->player);

    // Drawing.
    draw_begin(&state->drawer);

    draw_player(&state->player);

    draw_end();

    
}

void update_player(Player *p) {
    Vec2f vel = VEC2F_ORIGIN;

    
    if (is_hold_keycode(SDLK_d)) {
        vel.x += 1.0f;
    }
    if (is_hold_keycode(SDLK_a)) {
        vel.x -= 1.0f;
    }

    // Just for testing, this code is not properly working 2d movement, rewrite it late.
    vel = vec2f_multi_constant(vec2f_normalize(vel), p->speed);
    vel = vec2f_sum(vel, gravity_force);
    p->velocity = vec2f_lerp(p->velocity, vel, 0.1f);

    if (p->center.y - p->height / 2 <= 0 + jump_input_leeway && is_pressed_keycode(SDLK_SPACE)) {
        p->velocity.y += p->speed * 6;
    }

    p->center = vec2f_sum(p->center, vec2f_multi_constant(p->velocity, global_state->t->delta_time));

    if (p->center.y - p->height / 2 < 0) {
        p->center.y = p->height / 2;
    }

    

    // Debug draw lines.
    line_draw_begin(&global_state->line_drawer);
    draw_line(p->center, vec2f_sum(p->center, vel), VEC4F_CYAN);
    draw_line(p->center, vec2f_sum(p->center, p->velocity), VEC4F_RED);
    line_draw_end();

}

void draw_player(Player *p) {
    // Drawing player as a quad.
   draw_quad(vec2f_make(p->center.x - p->width / 2, p->center.y - p->height / 2), vec2f_make(p->center.x + p->width / 2, p->center.y + p->height / 2), VEC4F_YELLOW, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, 0.0f);
}





// Plug unload.
void plug_unload(Plug_State *state) {
    shader_unload(&(state->shader_table[hash_table_get_index_of(&state->shader_table, "quad", 4)]));
    shader_unload(&(state->shader_table[hash_table_get_index_of(&state->shader_table, "grid", 4)]));
    shader_unload(&(state->shader_table[hash_table_get_index_of(&state->shader_table, "line", 4)]));
    
    drawer_free(&state->drawer);
    line_drawer_free(&state->line_drawer);
    
    font_free(&state->font_table[hash_table_get_index_of(&state->font_table, "medium", 6)]);
    font_free(&state->font_table[hash_table_get_index_of(&state->font_table, "small", 5)]);
}


/**
 * Helper drawing functions.
 */
void draw_quad(Vec2f p0, Vec2f p1, Vec4f color, Texture *texture, Vec2f uv0, Vec2f uv1, Texture *mask, float offset_angle) {
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

    draw_quad_data(quad_data, 1);
}

void draw_text(const char *text, Vec2f current_point, Vec4f color, Font_Baked *font, u32 unit_scale) {
    u64 text_length = strlen(text);

    // Scale and adjust current_point.
    current_point = vec2f_multi_constant(current_point, (float)unit_scale);
    float origin_x = current_point.x;
    current_point.y += (float)font->baseline;

    // Text rendering variables.
    s32 font_char_index;
    u16 width, height;
    stbtt_bakedchar *c;

    for (u64 i = 0; i < text_length; i++) {
        // printf("%d\n", font_char_index);

        // @Incomplete: Handle special characters / symbols.
        if (text[i] == '\n') {
            current_point.x = origin_x;
            current_point.y -= (float)font->line_height;
            continue;
        }

        // Character drawing.
        font_char_index = (s32)text[i] - font->first_char_code;
        if (font_char_index < font->chars_count) {
            c = &font->chars[font_char_index];
            width  = font->chars[font_char_index].x1 - font->chars[font_char_index].x0;
            height = font->chars[font_char_index].y1 - font->chars[font_char_index].y0;

            draw_quad(vec2f_divide_constant(vec2f_make(current_point.x + c->xoff, current_point.y - c->yoff - height), (float)unit_scale), vec2f_divide_constant(vec2f_make(current_point.x + c->xoff + width, current_point.y - c->yoff), (float)unit_scale), color, NULL, vec2f_make(c->x0 / (float)font->bitmap.width, c->y1 / (float)font->bitmap.height), vec2f_make(c->x1 / (float)font->bitmap.width, c->y0 / (float)font->bitmap.height), &font->bitmap, 0.0f);

            current_point.x += font->chars[font_char_index].xadvance;
        }
    }
}

void draw_line(Vec2f p0, Vec2f p1, Vec4f color) {
    float line_data[14] = {
        p0.x, p0.y, 0.0f, color.x, color.y, color.z, color.w,
        p1.x, p1.y, 0.0f, color.x, color.y, color.z, color.w,
    };

    draw_line_data(line_data, 1);
}

void draw_function(float x0, float x1, Function y, u32 detail, Vec4f color) {
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
    
   
    draw_line_data(line_data, detail);
}

void draw_polar(float t0, float t1, Function r, u32 detail, Vec4f color) {
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
    
   
    draw_line_data(line_data, detail);
}

void draw_parametric(float t0, float t1, Function x, Function y, u32 detail, Vec4f color) {
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
    
   
    draw_line_data(line_data, detail);
}

void draw_area_function(float x0, float x1, Function y, u32 rect_count, Vec4f color) {
    float step = (x1 - x0) / (float)rect_count;
    for (u32 i = 0; i < rect_count; i++) {
        draw_quad(vec2f_make(x0 + step * i, 0.0f), vec2f_make(x0 + step * (i + 1), y(x0 + step * (i + 1))), color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, 0.0f);
    }
}

void draw_area_polar(float t0, float t1, Function r, u32 rect_count, Vec4f color) {
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
    
   
    draw_quad_data(quad_data, rect_count);
}

void draw_area_parametric(float t0, float t1, Function x, Function y, u32 rect_count, Vec4f color) {
    float step = (t1 - t0) / (float)rect_count;
    for (u32 i = 0; i < rect_count; i++) {
        draw_quad(vec2f_make(x(t0 + step * i), 0.0f), vec2f_make(x(t0 + step * (i + 1)), y(t0 + step * (i + 1))), color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, 0.0f);
    }
}


void draw_viewport(u32 x, u32 y, u32 width, u32 height, Vec4f color, Camera *camera) {
    glViewport(x, y, width, height);
    
    Vec2f p1 = vec2f_make((float)width / 2 / (float)camera->unit_scale, (float)height / 2 / (float)camera->unit_scale);
    Vec2f p0 = vec2f_negate(p1);
    
    draw_quad(p0, p1, color, NULL, p0, p1, NULL, 0.0f);

}

void viewport_reset(Plug_State *state) {
    glViewport(0, 0, state->window_width, state->window_height);
}









