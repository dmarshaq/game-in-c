#include "plug.h"
#include "core.h"
#include <string.h>



void draw_quad(Vec2f p0, Vec2f p1, Vec4f color, Texture *texture, Vec2f uv0, Vec2f uv1, Texture *mask, float offset_angle);
void draw_text(const char *text, Vec2f position, Vec4f color, Font_Baked *font, u32 unit_scale);
void draw_line(Vec2f p0, Vec2f p1, Vec4f color);
void draw_function(float x0, float x1, Function y, u32 detail, Vec4f color);
void draw_parametric(float t0, float t1, Function x, Function y, u32 detail, Vec4f color);

void draw_viewport(u32 x, u32 y, u32 width, u32 height, Vec4f color, Camera *camera);
void viewport_reset(Plug_State *state);


void plug_init(Plug_State *state) {
    s64 item;

    Hashmap hashmap = hashmap_make(s64, 16);
    hashmap_print(&hashmap);


    item = -12;
    hashmap_put(&hashmap, "apple", strlen("apple"), &item);
    // hashmap_print(&hashmap);


    item = 2;
    hashmap_put(&hashmap, "Orange", strlen("orange"), &item);
    // hashmap_print(&hashmap);


    item = 6;
    hashmap_put(&hashmap, "Banana", strlen("banana"), &item);
    // hashmap_print(&hashmap);


    item = 1;
    hashmap_put(&hashmap, "strawberry", strlen("strawberry"), &item);
    // hashmap_print(&hashmap);


    item = 10;
    hashmap_put(&hashmap, "candy", strlen("candy"), &item);
    hashmap_print(&hashmap);
}

void plug_load(Plug_State *state) {
    state->clear_color = vec4f_make(0.1f, 0.1f, 0.1f, 1.0f);

    // Main camera init.
    state->main_camera = camera_make(VEC2F_ORIGIN, 64);

    // Shader loading.
    state->quad_shader = shader_load("res/shader/quad.glsl");
    state->quad_shader.vertex_stride = 11;


    state->grid_shader = shader_load("res/shader/grid.glsl");
    state->grid_shader.vertex_stride = 11;
    shader_set_uniforms(&state->grid_shader);

    // Drawer init.
    drawer_init(&state->drawer, &state->quad_shader);


    // Shader loading.
    state->line_shader = shader_load("res/shader/line.glsl");
    state->line_shader.vertex_stride = 7;

    // Drawer init.
    line_drawer_init(&state->line_drawer, &state->line_shader);


    // Font loading.
    u8* font_data = read_file_into_buffer("res/font/font.ttf", NULL);

    state->font_baked_medium = font_bake(font_data, 20.0f);
    state->font_baked_small = font_bake(font_data, 16.0f);

    free(font_data);

}

void plug_unload(Plug_State *state) {
    shader_unload(&state->quad_shader);
    shader_unload(&state->line_shader);
    shader_unload(&state->grid_shader);

    drawer_free(&state->drawer);
    line_drawer_free(&state->line_drawer);


    font_free(&state->font_baked_medium);
    font_free(&state->font_baked_small);
}

void plug_update(Plug_State *state) {
    // Reset viewport
    viewport_reset(state);

    // Clear the screen with clear_color.
    glClearColor(state->clear_color.x, state->clear_color.y, state->clear_color.z, state->clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);

    // Updating projection.
    shader_update_projection(state->drawer.program, &state->main_camera, state->window_width, state->window_height); // @Incomplete: Supply of window dimensions.
    
    Vec2f p0, p1;
    p0 = vec2f_make(-8.0f, -5.0f);
    p1 = vec2f_make(8.0f, 5.0f);

    // Draw background grid.
    state->drawer.program = &state->grid_shader;
    draw_begin(&state->drawer);
    
    draw_quad(p0, p1, vec4f_make(0.9f, 0.9f, 0.9f, 0.2f), NULL, p0, p1, NULL, 0.0f);

    draw_end();
    
    // Drawing: Always between draw_begin() and draw_end().
    state->drawer.program = &state->quad_shader;
    draw_begin(&state->drawer);

    draw_text("As seen here, it is easy to debug any vectors just by drawing lines.\nMaybe in the future it also will be possible to plot graphs to debug\nsome game math related mechanics.", vec2f_make(-5.5f, 3.6f), VEC4F_CYAN, &state->font_baked_medium, state->main_camera.unit_scale);


    draw_text("Hello from hot reload!", vec2f_make(-1.5f, -3.6f), VEC4F_WHITE, &state->font_baked_medium, state->main_camera.unit_scale);

    draw_end();


    // Updating projection.
    shader_update_projection(state->line_drawer.program, &state->main_camera, state->window_width, state->window_height);


    float seconds = (u32)(state->t->current_time) % (u32)(4000 * PI) / 1000.0f;

    // Line Drawing
    line_draw_begin(&state->line_drawer);


    draw_function(-2*PI, seconds - 2*PI, func_expr(sinf(x)), 64, VEC4F_YELLOW);
    

    Vec2f cyan_vec;
    cyan_vec.x = 2 * cosf(state->angle);
    cyan_vec.y = 2 * sinf(state->angle);
    draw_line(VEC2F_ORIGIN, cyan_vec, VEC4F_CYAN);
    state->angle += PI / 12 * state->t->delta_time;

    draw_line(VEC2F_ORIGIN, VEC2F_RIGHT,    VEC4F_RED);
    draw_line(VEC2F_ORIGIN, VEC2F_UP,       VEC4F_GREEN);


    line_draw_end();

    // Drawing labels for vectors.
    draw_begin(&state->drawer);

    draw_text(" ( 1.00, 0.00 )", VEC2F_RIGHT,  VEC4F_WHITE, &state->font_baked_small, state->main_camera.unit_scale);
    draw_text(" ( 0.00, 1.00 )", VEC2F_UP,     VEC4F_WHITE, &state->font_baked_small, state->main_camera.unit_scale);

    char buffer[50];
    sprintf(buffer, " ( %2.2f, %2.2f )", cyan_vec.x, cyan_vec.y);
    draw_text(buffer, cyan_vec, VEC4F_WHITE, &state->font_baked_small, state->main_camera.unit_scale);


    draw_end();

    
    // Separate viewport example
    state->main_camera.unit_scale = 64;
    u32 height, width;
    height = state->window_height / 3; 
    width = state->window_width / 3; 

    shader_update_projection(state->drawer.program, &state->main_camera, width, height);
    shader_update_projection(state->line_drawer.program, &state->main_camera, width, height);
    shader_update_projection(&state->grid_shader, &state->main_camera, width, height);

    state->drawer.program = &state->grid_shader;
    draw_begin(&state->drawer);

    draw_viewport(25, 25, width, height, vec4f_make(0.3f, 0.3f, 0.3f, 1.0f), &state->main_camera);

    draw_end();
    state->drawer.program = &state->quad_shader;

    // Line Drawing
    line_draw_begin(&state->line_drawer);

    draw_parametric(-2*PI, seconds - 4*PI, func_expr(cosf(x)), func_expr(sinf(x)), 64, VEC4F_GREEN);

    line_draw_end();

    state->main_camera.unit_scale = 64;
    shader_update_projection(state->drawer.program, &state->main_camera, state->window_width, state->window_height);
    shader_update_projection(state->line_drawer.program, &state->main_camera, state->window_width, state->window_height);
    shader_update_projection(&state->grid_shader, &state->main_camera, state->window_width, state->window_height);

}

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



void draw_viewport(u32 x, u32 y, u32 width, u32 height, Vec4f color, Camera *camera) {
    glViewport(x, y, width, height);
    
    Vec2f p1 = vec2f_make((float)width / 2 / (float)camera->unit_scale, (float)height / 2 / (float)camera->unit_scale);
    Vec2f p0 = vec2f_negate(p1);
    
    draw_quad(p0, p1, color, NULL, p0, p1, NULL, 0.0f);

}

void viewport_reset(Plug_State *state) {
    glViewport(0, 0, state->window_width, state->window_height);
}









