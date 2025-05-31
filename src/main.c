#include "SDL2/SDL_events.h"
#include "SDL2/SDL_mouse.h"
#include "core.h"
#include "plug.h"
#include <SDL2/SDL_video.h>

#ifdef DEV
#include <windows.h>
#endif

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

const char* APP_NAME = "Game in C";

Time_Data t;

#ifdef DEV
bool reload_libplug();
bool plug_inited = false;

Plug_Init plug_init = NULL;
Plug_Load plug_load = NULL;
Plug_Unload plug_unload = NULL;
Plug_Update plug_update = NULL;
#endif

Plug_State state;

int main(int argc, char *argv[]) {

    #ifdef DEV
    fprintf(stdout, "Dev build is launched.\n");
    #endif

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

    keyboard_state_init();

    graphics_init();
    
    t.delta_time_multi = 1.0f;
    t.time_slow_factor = 1;

    state.t = &t;
    state.quit = false;
    state.window_width = WINDOW_WIDTH;
    state.window_height = WINDOW_HEIGHT;
    state.window = window;

    state.mouse_input = (Mouse_Input) {
        .position = VEC2F_ORIGIN,
        .left_hold = false,
        .left_pressed = false,
        .left_unpressed = false,
        .right_hold = false,
        .right_pressed = false,
        .right_unpressed = false,
    };

    #ifdef DEV
    reload_libplug();
    #else
    plug_init(&state);
    plug_load(&state);
    #endif


    SDL_Event event;

    t.last_update_time = 0; 
    t.update_step_time = 10; // Milliseconds per one update time
    while (!state.quit) {
        // Time management.
        t.current_time = SDL_GetTicks();
        t.accumilated_time += t.current_time - t.last_update_time;
        t.last_update_time = t.current_time;
        
        if (t.accumilated_time < t.update_step_time) {
            continue;
        }
        
        t.delta_time_milliseconds = (u32)((float)t.accumilated_time * t.delta_time_multi);
        t.delta_time = (float)(t.delta_time_milliseconds) / 1000.0f;
        t.accumilated_time = t.accumilated_time % t.update_step_time;


        state.mouse_input.left_pressed = false;
        state.mouse_input.left_unpressed = false;

        state.mouse_input.right_pressed = false;
        state.mouse_input.right_unpressed = false;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    state.quit = true;
                    break;
                case SDL_MOUSEMOTION:
                    s32 mx, my;
                    SDL_GetMouseState(&mx, &my);
                    
                    state.mouse_input.position.x = (float)mx;
                    state.mouse_input.position.y = (float)(state.window_height - my);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        state.mouse_input.left_pressed = true;
                        state.mouse_input.left_hold = true;
                    }
                    else if (event.button.button == SDL_BUTTON_RIGHT) {
                        state.mouse_input.right_pressed = true;
                        state.mouse_input.right_hold = true;
                    }
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        state.mouse_input.left_unpressed = true;
                        state.mouse_input.left_hold = false;
                    }
                    else if (event.button.button == SDL_BUTTON_RIGHT) {
                        state.mouse_input.right_unpressed = true;
                        state.mouse_input.right_hold = false;
                    }
                    break;
                default:
                    break;
            }
        }
        state.event = event;
        
        plug_update(&state);

        #ifdef DEV
        if (is_pressed_keycode(SDLK_r)) {
            if(!reload_libplug()) {
                fprintf(stderr, "%s Failed to hot reload plug.dll.\n", debug_error_str);
            }
        }
        #endif

        keyboard_state_old_update();
    }

    #ifndef DEV
    plug_unload(&state);
    #endif

    return 0;
}


#ifdef DEV

HMODULE mod = NULL;

bool reload_libplug() {
    const char *libplug_path = "bin/plug.dll";

    if (mod != NULL) {
        plug_unload(&state);
        FreeLibrary(mod);
        int result = system("make -B plug");
        printf("Hot reload compilation result: %d\n-----------------------------------------\n\n", result);
    }

    mod = LoadLibrary(libplug_path);
    if (mod == NULL) {
        fprintf(stderr, "%s Couldn't load dynamic library %s.\n", debug_error_str, libplug_path);
        return false;
    }

    plug_init = (Plug_Init)GetProcAddress(mod, "plug_init");
    if (plug_init == NULL) {
        fprintf(stderr, "%s Couldn't get adress plug_init function from %s.\n", debug_error_str, libplug_path);
        return false;
    }

    plug_load = (Plug_Load)GetProcAddress(mod, "plug_load");
    if (plug_load == NULL) {
        fprintf(stderr, "%s Couldn't get adress plug_load function from %s.\n", debug_error_str, libplug_path);
        return false;
    }

    plug_unload = (Plug_Unload)GetProcAddress(mod, "plug_unload");
    if (plug_unload == NULL) {
        fprintf(stderr, "%s Couldn't get adress plug_unload function from %s.\n", debug_error_str, libplug_path);
        return false;
    }

    plug_update = (Plug_Update)GetProcAddress(mod, "plug_update");
    if (plug_update == NULL) {
        fprintf(stderr, "%s Couldn't get adress plug_update function from %s.\n", debug_error_str, libplug_path);
        return false;
    }

    if (!plug_inited) {
        plug_init(&state);
        plug_inited = true;
    }

    plug_load(&state);
    return true;
}

#endif
