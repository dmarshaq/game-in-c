#include "core.h"
#include <stdio.h>

void draw_quad(Vec2f p0, Vec2f p1, Vec4f color, Texture *texture, Vec2f uv0, Vec2f uv1, Texture *mask, float offset_angle);
void draw_text(const char *text, Vec2f position, Vec4f color, Font_Baked *font);

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

const char* APP_NAME = "Spacejet";

Vec4f clear_color = vec4f_make(1.0f, 0.2f, 0.5f, 1.0f);
Camera main_camera;

Shader quad_shader;
Quad_Drawer drawer;

Font_Baked font_baked;

void start() {
    quad_shader = shader_load("res/shader/quad.glsl");
    quad_shader.vertex_stride = 11;
    drawer_init(&drawer, &quad_shader);

    clear_color = vec4f_make(0.2f, 0.4f, 0.2f, 1.0f);
    vec4f_print(clear_color);
    
    u64 font_size;
    u8* font_data = read_file_into_buffer("res/font/Nasa21-l23X.ttf", &font_size);

    font_baked = font_bake(font_data, 20.0f);

    free(font_data);

    main_camera = camera_make(VEC2F_ORIGIN, 64);
}

void update() {
    graphics_update_projection(&drawer, &main_camera, WINDOW_WIDTH, WINDOW_HEIGHT);

    
    draw_begin(&drawer);


    draw_text(" Spacejet Spacejet is a 2D structured game that is inspired by a combination\n of the roguelike genre and Galaga gameplay.\nMain goal is to write this game on C and develop a strong game development codebase for\n future projects. Development Timeline Codebase Create a new GitHub project. Create a simple project\nstructure. Game and framework, 2 file workflow. Float mathematics for game dev. \nGame loop, basic application. Foundation for graphics functionality. \nGeneric hashmap implementation.\n Advanced shader loading. Advanced texture loading. Better \ngraphics interface. Drawing interface. Immediate mode UI Development ", vec2f_make(-6.0f, 0.0f), vec4f_make(1.0f, 1.0f, 1.0f, 1.0f), &font_baked);
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
    while (!quit) {
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
    current_point = vec2f_multi_constant(current_point, 64);
    float origin_x = current_point.x;
    current_point.y += font->baseline;

    // Text rendering variables.
    s32 font_char_index;
    u16 width, height;
    stbtt_bakedchar *c;

    for (u64 i = 0; i < text_length; i++) {
        // printf("%d\n", font_char_index);

        // @Incomplete: Handle special characters / symbols.
        if (text[i] == '\n') {
            current_point.x = origin_x;
            current_point.y -= font->line_height;
            continue;
        }

        // Character drawing.
        font_char_index = (s32)text[i] - font->first_char_code;
        if (font_char_index < font->chars_count) {
            c = &font->chars[font_char_index];
            width  = font->chars[font_char_index].x1 - font->chars[font_char_index].x0;
            height = font->chars[font_char_index].y1 - font->chars[font_char_index].y0;

            draw_quad(vec2f_divide_constant(vec2f_make(current_point.x + c->xoff, current_point.y - c->yoff - height), 64), vec2f_divide_constant(vec2f_make(current_point.x + c->xoff + width, current_point.y - c->yoff), 64), color, NULL, vec2f_make(c->x0 / (float)font->bitmap.width, c->y1 / (float)font->bitmap.height), vec2f_make(c->x1 / (float)font->bitmap.width, c->y0 / (float)font->bitmap.height), &font->bitmap, 0.0f);

            current_point.x += font->chars[font_char_index].xadvance;
        }
    }
}


