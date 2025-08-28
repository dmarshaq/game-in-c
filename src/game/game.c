#include "game/game.h"

#include "core/core.h"
#include "core/type.h"
#include "core/structs.h"
#include "core/file.h"
#include "core/mathf.h"
#include "core/typeinfo.h"

#include "game/graphics.h"
#include "game/input.h"
#include "game/draw.h"
#include "game/event.h"
#include "game/console.h"
#include "game/editor.h"
#include "game/command.h"
#include "game/vars.h"
#include "game/imui.h"
#include "game/asset.h"

#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_keyboard.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>






/**
 * Global state.
 *
 * @Important: Address of state structure doesn't change no matter what.
 * It exists through out the game execution and it's data changes accordingly to the current game state.
 */
static State *state;



// Keybinds (Simple).
// @Temporary: Will be replaced with meta program + keybinds config solution.
#define BIND_PRESSED(keybind)   (!SDL_IsTextInputActive() && pressed(keybind))
#define BIND_HOLD(keybind)      (!SDL_IsTextInputActive() && hold(keybind))
#define BIND_UNPRESSED(keybind) (!SDL_IsTextInputActive() && unpressed(keybind))










// File formats.

static const String SHADER_FILE_FORMAT  = STR_BUFFER("glsl");
static const String FONT_FILE_FORMAT    = STR_BUFFER("ttf");



void game_init(State *global_state) {
    /**
     * The pointer to global state is copied only once and stored internally in this file.
     * @Important: It assumes, that this address will remain the same through ou the game and will not change no matter what.
     */
    state = global_state;


    // Setting random seed.
    srand((u32)time(NULL));


    // Init vars.
    vars_tree_begin();


    // Init assets observer.
    if (asset_observer_init("res") != 0) {
        printf_err("Couldn't init Asset Observer.\n");
        exit(1);
    }
    
    // Force load asset changes.
    if (asset_force_changes("res") != 0) {
        printf_err("Couldn't force load asset changes.\n");
        exit(1);
    }
    
    // Getting asset changes.
    u32 changes_count;
    const Asset_Change *changes;

    if (!asset_view_changes(&changes_count, &changes)) {
        printf_err("Couldn't view loaded asset changes.\n");
        exit(1);
    }



    // Init SDL and GL.
    if (init_sdl_gl()) {
        fprintf(stderr, "%s Couldn't init SDL and GL.\n", debug_error_str);
        exit(1);
    }
    
    // Init audio.
    if (init_sdl_audio()) {
        fprintf(stderr, "%s Couldn't init audio.\n", debug_error_str);
        exit(1);
    }


    // Create window.
    state->window = create_gl_window("Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 700);

    if (state->window.ptr == NULL) {
        fprintf(stderr, "%s Couldn't create window.\n", debug_error_str);
        exit(1);
    }




    // Initing graphics.
    graphics_init();

    // Keyboard input init.
    keyboard_state_init();

    // Initing events.
    event_init_handler(&state->events);




    /**
     * Setting hash tables for resources. 
     * Since they are dynamically allocated pointers will not change after hot reloading.
     */
    state->shader_table = hash_table_make(Shader, 8, &std_allocator);


    /**
     * This just goes through asset changes that are forced by 'asset_force_changes(...).
     * And loads each one using specific loading function.
     * The idea is to have each module that needs some resources to be loaded provide it's own loading function for one file at a time.
     * And such functions then can simply be called here.
     */

    // Loading Shaders specifically.
    for (u32 i = 0; i < changes_count; i++) {
        if (str_equals(changes[i].file_format, SHADER_FILE_FORMAT)) {
            printf("Detected Shader Asset: '%.*s'\n", UNPACK(changes[i].full_path));
            char _buffer[changes[i].full_path.length + 1]; 
            str_copy_to(changes[i].full_path, _buffer);
            _buffer[changes[i].full_path.length]     = '\0';

            Shader shader = shader_load(_buffer);
            shader_init_uniforms(&shader);

            String shader_name = str_substring(changes[i].file_name, 0, str_find_char_right(changes[i].file_name, '.'));
            hash_table_put(&state->shader_table, shader, UNPACK(shader_name));

            asset_remove_change(i);
            i--;
            changes_count--;

            continue;
        }
    }



    // Drawers init.
    drawer_init(&state->quad_drawer, hash_table_get(&state->shader_table, UNPACK_LITERAL("quad")));



    // Main camera init.
    state->main_camera = camera_make(VEC2F_ORIGIN, 48);


    // Init commands.
    command_init();

    // Init console.
    console_init(state);

    // Init editor.
    editor_init(state);







    


    // Finishing vars tree.
    state->vars_tree = vars_tree_build();

    // Loading Vars files specifically. After the vars tree is built...
    for (u32 i = 0; i < changes_count; i++) {
        if (str_equals(changes[i].file_format, VARS_FILE_FORMAT)) {
            printf("Detected Vars Asset: '%.*s'\n", UNPACK(changes[i].full_path));

            vars_load_file(changes[i].full_path, &state->vars_tree);

            asset_remove_change(i);
            i--;
            changes_count--;

            continue;
        }
    }









    // Setting clear color.
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);

    // Logging hello world to the console.
    console_log("Hello world!\n");
    
}

/**
 * @Important: In game update loops the order of procedures is: Updating -> Drawing.
 * Where in updating all logic of the game loop is contained including inputs, sound and so on.
 * Drawing part is only responsible for putting pixels accrodingly with calculated data in "Updating" part.
 */
void game_update() {
    // Polling any asset changes.
    if (asset_observer_poll_changes() != 0) {
        printf_err("Couldn't poll asset changes.\n");
        exit(1);
    }

    // Listen to any vars files changed.
    vars_listen_to_changes(state->vars_tree);
    
    // Handling events
    event_handle(&state->events, &state->window, &state->t);

    // Clearin screen.
    glClear(GL_COLOR_BUFFER_BIT);
    



    // Matrix4f projection = camera_calculate_projection(&state->main_camera, state->window.width, state->window.height);

    // shader_update_projection(state->quad_drawer.program, &projection);

    // draw_begin(&state->quad_drawer);

    // draw_quad(vec2f_make(-1.0f, -1.0f), vec2f_make(1.0f, 1.0f));

    // draw_end();




    // Editor drawing.
    editor_draw(&state->window);

    // Console update.
    console_update(&state->window, &state->events, &state->t);
    // Console drawing.
    console_draw(&state->window);
   


    // Checking for gl error.
    check_gl_error();


    // Swap buffers to display the rendered image.
    SDL_GL_SwapWindow(state->window.ptr);


    // Post updating input.
    keyboard_state_old_update();
}

void game_free() {
    console_free();

    shader_unload(hash_table_get(&state->shader_table, UNPACK_LITERAL("quad")));
    drawer_free(&state->quad_drawer);
}

void quit() {
    state->events.should_quit = true;
}














