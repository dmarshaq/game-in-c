#include "core.h"

Vec4f color = vec4f_make(1.0f, 0.2f, 0.5f, 1.0f);

void start() {
    Matrix4f proj = matrix4f_orthographics(-2.0f, 2.0f, -1.0f, 1.0f, -1.0f, 1.0f);
    matrix4f_print(proj);
    
    color = vec4f_make(0.2f, 0.4f, 0.2f, 1.0f);
    vec4f_print(color);
}

void update() {
    
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

    SDL_Window *window = create_gl_window("Spacejet", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600);
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

                update();
            }
        }

        // Clear the screen with color.
        glClearColor(color.x, color.y, color.z, color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Swap buffers to display the rendered image.
        SDL_GL_SwapWindow(window);
    }

    return 0;
}
