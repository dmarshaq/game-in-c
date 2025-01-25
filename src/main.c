#include "core.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

const char* APP_NAME = "Spacejet";

Vec4f clear_color = vec4f_make(1.0f, 0.2f, 0.5f, 1.0f);
Camera main_camera;

Shader quad_shader;
Quad_Drawer drawer;

void start() {
    quad_shader = shader_load("res/shader/quad.glsl");
    quad_shader.vertex_stride = 10;
    drawer_init(&drawer, &quad_shader);

    clear_color = vec4f_make(0.2f, 0.4f, 0.2f, 0.2f);
    vec4f_print(clear_color);

    main_camera = camera_make(VEC2F_ORIGIN, 32);
}

void update() {
    graphics_update_projection(&drawer, &main_camera, WINDOW_WIDTH, WINDOW_HEIGHT);
    float quad_data[40] = {
        -1.5f, -1.5f,  0.0f,  1.0f,  0.2f,  0.2f,  1.0f,  0.0f,  0.0f, -1.0f,
         1.5f, -1.5f,  0.0f,  1.0f,  0.2f,  0.2f,  1.0f,  1.0f,  0.0f, -1.0f,
        -1.5f,  1.5f,  0.0f,  1.0f,  0.2f,  0.2f,  1.0f,  0.0f,  1.0f, -1.0f,
         1.5f,  1.5f,  0.0f,  1.0f,  0.2f,  0.2f,  1.0f,  1.0f,  1.0f, -1.0f,
    };
    float *ptr = quad_data;

    draw_quad(&drawer, ptr);

    draw(&drawer);
    draw_clean();
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
