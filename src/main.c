#include "core/core.h"
#include "core/structs.h"
#include "game/plug.h"
#include "game/graphics.h"
#include "game/input.h"

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_video.h>

#ifdef DEV
#include <windows.h>
#endif


#ifdef DEV
bool reload_libplug();
bool plug_inited = false;

Plug_Init plug_init = NULL;
Plug_Load plug_load = NULL;
Plug_Unload plug_unload = NULL;
Plug_Update plug_update = NULL;
#endif

static Plug_State *state;

int main(int argc, char *argv[]) {

    #ifdef DEV
    fprintf(stdout, "Dev build is launched.\n");
    #endif

    // Allocate global state.
    state = allocator_alloc(&std_allocator, sizeof(Plug_State));
    
    state->t.delta_time_multi = 1.0f;
    state->t.time_slow_factor = 1;

    #ifdef DEV
    state->should_hot_reload = false;
    reload_libplug();
    #else
    plug_init(state);
    plug_load(state);
    #endif


    state->t.last_update_time = 0; 
    state->t.update_step_time = 10; // Milliseconds per one update time
    while (true) {
        // Time management.
        state->t.current_time = SDL_GetTicks();
        state->t.accumilated_time += state->t.current_time - state->t.last_update_time;
        state->t.last_update_time = state->t.current_time;
        
        if (state->t.accumilated_time < state->t.update_step_time) {
            continue;
        }
        
        state->t.delta_time_milliseconds = (u32)((float)state->t.accumilated_time * state->t.delta_time_multi);
        state->t.delta_time = (float)(state->t.delta_time_milliseconds) / 1000.0f;
        state->t.accumilated_time = state->t.accumilated_time % state->t.update_step_time;

        
        plug_update(state);

        #ifdef DEV
        if (state->should_hot_reload) {
            if(!reload_libplug()) {
                fprintf(stderr, "%s Failed to hot reload plug.dll.\n", debug_error_str);
            }
        }
        #endif


        if (state->events.should_quit) {
            break;
        }
    }

    #ifndef DEV
    plug_unload(state);
    #endif

    return 0;
}


#ifdef DEV

HMODULE mod = NULL;

bool reload_libplug() {
    const char *libplug_path = "bin/plug.dll";

    if (mod != NULL) {
        plug_unload(state);
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
        plug_init(state);
        plug_inited = true;
    }

    plug_load(state);
    return true;
}

#endif
