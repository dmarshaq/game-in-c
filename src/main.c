#include "core.h"
#include <math.h>
#include <stdio.h>

void draw_quad(Vec2f p0, Vec2f p1, Vec4f color, Texture *texture, Vec2f uv0, Vec2f uv1, Texture *mask, float offset_angle);
void draw_text(const char *text, Vec2f position, Vec4f color, Font_Baked *font);
void draw_line(Vec2f p0, Vec2f p1, Vec4f color);

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

typedef struct time_data {
    u32 current_time;
    u32 delta_time_milliseconds;
    float delta_time;

    u32 last_update_time;
    u32 accumilated_time;
    u32 update_step_time;
} Time_Data;

const char* APP_NAME = "Game in C";

Time_Data t;
Vec4f clear_color = vec4f_make(0.2f, 0.2f, 0.2f, 1.0f);
Camera main_camera;

Shader quad_shader;
Quad_Drawer drawer;

Font_Baked font_baked_medium;
Font_Baked font_baked_small;


Shader line_shader;
Line_Drawer line_drawer;

void start() {
    // Shader loading.
    quad_shader = shader_load("res/shader/quad.glsl");
    quad_shader.vertex_stride = 11;

    // Drawer init.
    drawer_init(&drawer, &quad_shader);


    // Shader loading.
    line_shader = shader_load("res/shader/line.glsl");
    line_shader.vertex_stride = 7;

    // Drawer init.
    line_drawer_init(&line_drawer, &line_shader);


    // Font loading.
    u8* font_data = read_file_into_buffer("res/font/font.ttf", NULL);
    font_baked_medium = font_bake(font_data, 20.0f);
    font_baked_small = font_bake(font_data, 16.0f);
    free(font_data);
    
    // Main camera init.
    main_camera = camera_make(VEC2F_ORIGIN, 64);
}

float angle = 0;
Vec2f cyan_vec;
char buffer[50];

void update() {
    // Updating projection.
    graphics_update_projection(drawer.program, &main_camera, WINDOW_WIDTH, WINDOW_HEIGHT);


    // Drawing: Always between draw_begin() and draw_end().
    draw_begin(&drawer);

    // draw_text("game-in-c is a 2D structured game that will serve as a showcase product of\na simultaneously developing code base. Main goal is to write this game on C\nand create a game development codebase for future projects.", vec2f_make(-6.0f, 0.0f), vec4f_make(sinf((float)t.current_time / 1440.0f - 0.0f), sinf((float)t.current_time / 1660.0f - 800.0f), sinf((float)t.current_time / 2200.0f - 1200.0f), 1.0f), &font_baked_medium);

    draw_text("As seen here, it is easy to debug any vectors just by drawing lines.\nMaybe in the future it also will be possible to plot graphs to debug\nsome game math related mechanics.", vec2f_make(-5.5f, 3.6f), VEC4F_YELLOW, &font_baked_medium);

    draw_end();


    // Updating projection.
    graphics_update_projection(line_drawer.program, &main_camera, WINDOW_WIDTH, WINDOW_HEIGHT);


    // Line Drawing
    line_draw_begin(&line_drawer);

    cyan_vec.x = 2 * cosf(angle);
    cyan_vec.y = 2 * sinf(angle);
    draw_line(VEC2F_ORIGIN, cyan_vec, VEC4F_CYAN);
    angle += PI / 12 * t.delta_time;

    draw_line(VEC2F_ORIGIN, VEC2F_RIGHT,    VEC4F_RED);
    draw_line(VEC2F_ORIGIN, VEC2F_UP,       VEC4F_GREEN);


    line_draw_end();

    // Drawing labels for vectors.
    draw_begin(&drawer);

    draw_text(" ( 1.00, 0.00 )", VEC2F_RIGHT,  VEC4F_WHITE, &font_baked_small);
    draw_text(" ( 0.00, 1.00 )", VEC2F_UP,     VEC4F_WHITE, &font_baked_small);

    sprintf(buffer, " ( %2.2f, %2.2f )", cyan_vec.x, cyan_vec.y);
    draw_text(buffer, cyan_vec, VEC4F_WHITE, &font_baked_small);

    draw_end();
}


int main(int argc, char *argv[]) {
    if (init_sdl_gl()) {
        fprintf(stderr, "%s Couldn't init SDL and GL.\n", debug_error_str);
        return 1;
    }
    
    if (init_sdl_audio()) {
        fprintf(stderr, "%s Couldn't init audio.\n", debug_error_str);
        return 1;
    }

    SDL_Window *window = create_gl_window(APP_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT);
    if (window == NULL) {
        fprintf(stderr, "%s Couldn't create window.\n", debug_error_str);
        return 1;
    }

    bool quit = false;
    
    graphics_init();

    start();

    SDL_Event event;

    t.last_update_time = 0; 
    t.update_step_time = 10; // Milliseconds per one update time
    while (!quit) {
        // Time management.
        t.current_time = SDL_GetTicks();
        t.accumilated_time += t.current_time - t.last_update_time;
        t.last_update_time = t.current_time;
        
        if (t.accumilated_time < t.update_step_time) {
            continue;
        }
        
        t.delta_time_milliseconds = t.accumilated_time;
        t.delta_time = (float)(t.delta_time_milliseconds) / 1000.0f;
        t.accumilated_time = t.accumilated_time % t.update_step_time;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                default:
                    break;
            }
        }
        // Clear the screen with clear_color.
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        update();

        // Swap buffers to display the rendered image.
        SDL_GL_SwapWindow(window);
    }

    return 0;
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

void draw_text(const char *text, Vec2f current_point, Vec4f color, Font_Baked *font) {
    u64 text_length = strlen(text);

    // Scale and adjust current_point.
    current_point = vec2f_multi_constant(current_point, (float)main_camera.unit_scale);
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

            draw_quad(vec2f_divide_constant(vec2f_make(current_point.x + c->xoff, current_point.y - c->yoff - height), (float)main_camera.unit_scale), vec2f_divide_constant(vec2f_make(current_point.x + c->xoff + width, current_point.y - c->yoff), (float)main_camera.unit_scale), color, NULL, vec2f_make(c->x0 / (float)font->bitmap.width, c->y1 / (float)font->bitmap.height), vec2f_make(c->x1 / (float)font->bitmap.width, c->y0 / (float)font->bitmap.height), &font->bitmap, 0.0f);

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


