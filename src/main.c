#include "core.h"

void draw_quad(Vec2f p0, Vec2f p1, Vec4f color, Texture *texture, Vec2f uv0, Vec2f uv1, Texture *mask, float offset_angle);
void draw_text(const char *text, Vec2f position, Vec4f color, Font_Baked *font);

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 900

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

    clear_color = vec4f_make(0.2f, 0.4f, 0.2f, 0.2f);
    vec4f_print(clear_color);
    
    u64 font_size;
    u8* font_data = read_file_into_buffer("res/font/font.ttf", &font_size);

    font_baked = font_bake(font_data, 72.0f);

    free(font_data);

    main_camera = camera_make(VEC2F_ORIGIN, 64);
}

void update() {
    graphics_update_projection(&drawer, &main_camera, WINDOW_WIDTH, WINDOW_HEIGHT);

    
    draw_begin(&drawer);

    float size = 8.0f;

    draw_quad(VEC2F_ORIGIN, vec2f_make(size, size), vec4f_make(0.3f, 0.2f, 0.3f, 1.0f), NULL, vec2f_make(0.0f, 1.0f), vec2f_make(1.0f, 0.0f), NULL, 0.0f);
    draw_quad(VEC2F_ORIGIN, vec2f_make(size, size), vec4f_make(1.0f, 1.0f, 1.0f, 0.5f), NULL, vec2f_make(0.0f, 1.0f), vec2f_make(1.0f, 0.0f), &font_baked.bitmap, 0.0f);

    draw_text("''Hello World! gy?()", VEC2F_ORIGIN, vec4f_make(1.0f, 1.0f, 1.0f, 1.0f), &font_baked);

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

    draw_quad_data(quad_data);
}

void draw_text(const char *text, Vec2f origin, Vec4f color, Font_Baked *font) {
    u64 text_length = strlen(text);

    for (u64 i = 0; i < text_length; i++) {
        // @Incomplete: Handle special characters / symbols.

        // Character drawing.
        
        s32 font_char_index = (s32)text[i] - font->first_char_code;
        u16 width, height;
        stbtt_bakedchar *c;
        if (font_char_index < font->chars_count) {
            c = &font->chars[font_char_index];
            width  = font->chars[font_char_index].x1 - font->chars[font_char_index].x0;
            height = font->chars[font_char_index].y1 - font->chars[font_char_index].y0;



            // printf("index: %3d -> x0: %3d, y0: %3d, x1: %3d, y1: %3d, xoff: %2.4f, yoff: %2.2f, xadvance: %2.2f, width: %3d, height: %3d.\n", font_char_index, c->x0, c->y0, c->x1, c->y1, c->xoff, c->yoff, c->xadvance, width, height);
            
            draw_quad(vec2f_sum(origin, vec2f_divide_constant(vec2f_make(c->xoff, -height - c->yoff), 64)), vec2f_sum(origin, vec2f_divide_constant(vec2f_make(c->xoff + width, -c->yoff), 64)), color, NULL, vec2f_make(c->x0 / (float)font->bitmap.width, c->y1 / (float)font->bitmap.height), vec2f_make(c->x1 / (float)font->bitmap.width, c->y0 / (float)font->bitmap.height), &font->bitmap, 0.0f);

            origin.x += font->chars[font_char_index].xadvance / 64;
        }
    }
}


