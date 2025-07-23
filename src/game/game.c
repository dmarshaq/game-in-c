#include "game/game_meta_.h"

#include "core/core.h"
#include "core/type.h"
#include "core/structs.h"
#include "core/file.h"
#include "core/mathf.h"

#include "game/graphics_meta_.h"
#include "game/input_meta_.h"
#include "game/draw_meta_.h"
#include "game/event_meta_.h"
#include "game/console_meta_.h"
#include "game/vars_meta_.h"
#include "game/imui_meta_.h"

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
 * Constants.
 */
static const u8 MAX_BOXES = 32;
static const float MAX_PLAYER_SPEED = 6.0f;




// Global state.
static State *state;







// Keybinds (Simple).
static const s32 KEYBIND_PLAYER_JUMP       = SDLK_SPACE;
static const s32 KEYBIND_PLAYER_MOVE_RIGHT = SDLK_d;
static const s32 KEYBIND_PLAYER_MOVE_LEFT  = SDLK_a;
static const s32 KEYBIND_PLAYER_ATTACK     = SDLK_q;
static const s32 KEYBIND_MENU_ENTER        = SDLK_m;
static const s32 KEYBIND_MENU_EXIT         = SDLK_m;
static const s32 KEYBIND_TIME_TOGGLE       = SDLK_t;
static const s32 KEYBIND_CONSOLE_TOGGLE    = SDLK_BACKQUOTE;
static const s32 KEYBIND_CONSOLE_ENTER     = SDLK_RETURN;

// #define BIND_PRESSED(keybind)   (!state->events.text_input.enabled && pressed(keybind))
// #define BIND_HOLD(keybind)      (!state->events.text_input.enabled && hold(keybind))
// #define BIND_UNPRESSED(keybind) (!state->events.text_input.enabled && unpressed(keybind))
#define BIND_PRESSED(keybind)   (!SDL_IsTextInputActive() && pressed(keybind))
#define BIND_HOLD(keybind)      (!SDL_IsTextInputActive() && hold(keybind))
#define BIND_UNPRESSED(keybind) (!SDL_IsTextInputActive() && unpressed(keybind))


/**
 * Global state functions.
 */

void quit() {
    state->events.should_quit = true;
}












/**
 * Console.
 */

// void cprintf()
// 
// void cprintf(char *format, ...) {
//     va_list args;
//     va_start(args, format);
//     s32 bytes_written = vsnprintf(pop_up_buffer + pop_up_buffer_start, sizeof(pop_up_buffer) - pop_up_buffer_start, format, args);
//     va_end(args);
//     pop_up_lerp_t = 0.0f;
// 
//     if (bytes_written > 0)
//         pop_up_buffer_start += bytes_written;
// }



/**
 * Pop up message.
 */

// static char pop_up_buffer[256];
// static u32 pop_up_buffer_start = 0;
// static float pop_up_lerp_t = 0.0f;
// 
// 
// void pop_up_log(char *format, ...) {
//     va_list args;
//     va_start(args, format);
//     s32 bytes_written = vsnprintf(pop_up_buffer + pop_up_buffer_start, sizeof(pop_up_buffer) - pop_up_buffer_start, format, args);
//     va_end(args);
//     pop_up_lerp_t = 0.0f;
// 
//     if (bytes_written > 0)
//         pop_up_buffer_start += bytes_written;
// }
// 
// 
// /**
//  * Draws pop_up text that is stored in "pop_up_mesg", doesn't draw anything if it is NULL.
//  * Use "pop_up_log(char *format, ...)" to log message;
//  */
// void draw_pop_up(Vec2f position, Vec4f color, Font_Baked *font, u32 unit_scale) {
//     if (pop_up_buffer[0] == '\0')
//         return;
//     
//     color.w = lerp(color.w, 0.0f, pop_up_lerp_t);
//     position.y = lerp(position.y, position.y + 25.0f, pop_up_lerp_t);
// 
//     draw_text(pop_up_buffer, position, color, font, unit_scale, NULL);
// 
//     pop_up_lerp_t += 0.002f;
// 
//     if (pop_up_lerp_t >= 1.0f) {
//         pop_up_buffer[0] = '\0';
//         pop_up_lerp_t = 0.0f;
//     }
// 
//     pop_up_buffer_start = 0;
// }

































/**
 * Box functions.
 */

void spawn_box(Vec2f position, Vec4f color) {
    u32 free_index = 0;

    for (; free_index < MAX_BOXES; free_index++) {
        if (state->boxes[free_index].destroyed) {
            break;
        }   
    }

    if (free_index == MAX_BOXES) {
        printf_err("Max boxes limit have been reached, cannot spawn more boxes.\n");
        return;
    }

    float width = 0.6f * randf() + 0.8f;
    float height = 0.6f * randf() + 0.8f;
    

    state->boxes[free_index] = (Box) {
        .p_box = (Phys_Box) {
                .bound_box = obb_make(position, width, height, 0.0f),
                .body = body_obb_make(30.0f, position, width, height, 0.4f, 0.45f, 0.3f),

                .dynamic = true,
                .rotatable = true,
                .destructible = false,
                .gravitable = true,
                .grounded = false,
            },
        .color = color,
        .destroyed = false,
    };

    array_list_append(&state->phys_boxes, &state->boxes[free_index].p_box);
}

void spawn_obstacle(Vec2f c, float w, float h, Vec4f color, float angle) {
    u32 free_index = 0;

    for (; free_index < MAX_BOXES; free_index++) {
        if (state->boxes[free_index].destroyed) {
            break;
        }   
    }

    if (free_index == MAX_BOXES) {
        printf_err("Max boxes limit have been reached, cannot spawn more boxes.\n");
        return;
    }

    state->boxes[free_index] = (Box) {
        .p_box = (Phys_Box) {
                .bound_box = obb_make(c, w, h, angle),
                .body = body_obb_make(0.0f, c, w, h, 0.4f, 0.45f, 0.3f),

                .dynamic = false,
                .destructible = false,
                .rotatable = true,
                .gravitable = false,
                .grounded = false,
            },
        .color = color,
        .destroyed = false,
    };

    array_list_append(&state->phys_boxes, &state->boxes[free_index].p_box);
}

void spawn_rect(Vec2f p0, Vec2f p1, Vec4f color) {
    u32 free_index = 0;

    for (; free_index < MAX_BOXES; free_index++) {
        if (state->boxes[free_index].destroyed) {
            break;
        }   
    }

    if (free_index == MAX_BOXES) {
        printf_err("Max boxes limit have been reached, cannot spawn more boxes.\n");
        return;
    }

    state->boxes[free_index] = (Box) {
        .p_box = (Phys_Box) {
                .bound_box = obb_make(vec2f_sum(p0, vec2f_divide_constant(vec2f_difference(p1, p0), 2)), p1.x - p0.x, p1.y - p0.y, 0.0f),
                .body = body_obb_make(0.0f, vec2f_sum(p0, vec2f_divide_constant(vec2f_difference(p1, p0), 2)), p1.x - p0.x, p1.y - p0.y, 0.4f, 0.45f, 0.3f),

                .dynamic = false,
                .destructible = false,
                .rotatable = true,
                .gravitable = false,
                .grounded = false,
            },
        .color = color,
        .destroyed = false,
    };

    array_list_append(&state->phys_boxes, &state->boxes[free_index].p_box);
}

void update_boxes() {
    for (u32 i = 0; i < MAX_BOXES; i++) {
        if (state->boxes[i].destroyed) {
            continue;
        }
        if (state->boxes[i].p_box.bound_box.center.y < -15.0f) {
            state->boxes[i].p_box.bound_box.dimensions.x = 0.0f;
            state->boxes[i].destroyed = true;

        }
    }
}

void draw_boxes() {
    for (u32 i = 0; i < MAX_BOXES; i++) {
        if (!state->boxes[i].destroyed)
            draw_quad(obb_p0(&state->boxes[i].p_box.bound_box), obb_p1(&state->boxes[i].p_box.bound_box), state->boxes[i].color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, state->boxes[i].p_box.bound_box.rot, NULL);
    }
}

void draw_boxes_outline() {
    for (u32 i = 0; i < MAX_BOXES; i++) {
        if (!state->boxes[i].destroyed)
            draw_quad_outline(obb_p0(&state->boxes[i].p_box.bound_box), obb_p1(&state->boxes[i].p_box.bound_box), vec4f_make(1.0f, 1.0f, 1.0f, 0.6f), state->boxes[i].p_box.bound_box.rot, NULL);
    }
}











/**
 * Player functions.
 */

void spawn_player(Vec2f position, Vec4f color) {
    state->player = (Player) {
        .p_box = (Phys_Box) {
                .bound_box = obb_make(position, 1.0f, 1.8f, 0.0f),
                .body = body_obb_make(30.0f, position, 1.0f, 1.8f, 0.0f, 0.45f, 0.3f),
                .dynamic = true,
                .rotatable = false,
                .destructible = false,
                .gravitable = true,
                .grounded = false,
                },
        .color = color,
        .speed = 0.0f,


        .sword = (Sword) {
            .origin = transform_make_trs_2d(position, 0.0f, vec2f_make(1.0f, 1.0f)),
            .handle = transform_make_trs_2d(position, 0.0f, vec2f_make(2.8f, 1.0f)),

            .angle_a = -30.0f,
            .angle_b = 60.0f,
            .animator = ti_make(0.2f),
        }   
    };

    array_list_append(&state->phys_boxes, &state->player.p_box);
}

void update_player(Player *p) {
    float x_vel = 0.0f;
    if (BIND_HOLD(KEYBIND_PLAYER_MOVE_RIGHT)) {
        x_vel = 1.0f;

        transform_set_flip_x(&p->sword.handle, 1.0f);
        transform_set_flip_x(&p->sword.origin, 1.0f);
    } else if (BIND_HOLD(KEYBIND_PLAYER_MOVE_LEFT)) {
        x_vel = -1.0f;

        transform_set_flip_x(&p->sword.handle, -1.0f);
        transform_set_flip_x(&p->sword.origin, -1.0f);
    }


    p->p_box.body.velocity.x = lerp(p->p_box.body.velocity.x, x_vel * p->speed, 0.5f);

    if (BIND_PRESSED(KEYBIND_PLAYER_JUMP) && p->p_box.grounded) {
        phys_apply_force(&p->p_box.body, vec2f_make(0.0f, 140.0f));
    }


    // Applying calculated velocities. For both AABB and Rigid Body 2D.
    p->p_box.bound_box.center = vec2f_sum(p->p_box.bound_box.center, vec2f_multi_constant(p->p_box.body.velocity, state->t.delta_time));
    p->p_box.body.mass_center = p->p_box.bound_box.center;

    // Updating sword.
    transform_set_rotation_2d(&p->sword.origin, deg2rad(lerp(p->sword.angle_a, p->sword.angle_b, ease_in_back(ti_elapsed_percent(&p->sword.animator))) ));
    transform_set_rotation_2d(&p->sword.handle, deg2rad(lerp(p->sword.angle_a, p->sword.angle_b, ease_in_back(ti_elapsed_percent(&p->sword.animator))) ));

    transform_set_translation_2d(&p->sword.origin, p->p_box.bound_box.center);
    transform_set_translation_2d(&p->sword.handle, matrix4f_mul_vec2f(p->sword.origin, VEC2F_RIGHT));

    ti_update(&p->sword.animator, state->t.delta_time);


    if (BIND_PRESSED(KEYBIND_PLAYER_ATTACK)) {
        float temp = p->sword.angle_a;
        p->sword.angle_a = p->sword.angle_b;
        p->sword.angle_b = temp;

        ti_reset(&p->sword.animator);
    }
}

void draw_player(Player *p) {
    // Drawing player as a quad.
    draw_quad(obb_p0(&p->p_box.bound_box), obb_p1(&p->p_box.bound_box), p->color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, p->p_box.bound_box.rot, NULL);
}











/**
 * Sword functions.
 */

void draw_sword_trail(Sword *s) {

}

void draw_sword_line(Sword *s) {
    // Drawing sword as a stick.
    Vec2f right = matrix4f_mul_vec2f(s->handle, VEC2F_RIGHT);
    Vec2f up = matrix4f_mul_vec2f(s->handle, VEC2F_UP);
    Vec2f origin = matrix4f_mul_vec2f(s->handle, VEC2F_ORIGIN);

    draw_line(origin, right, VEC4F_RED, NULL);
    draw_line(origin, up, VEC4F_GREEN, NULL);
}













void game_update() {

    /**
     * -----------------------------------
     *  Updating
     * -----------------------------------
     */


    // Spawning box.
    if (state->events.mouse_input.right_pressed) {
        Vec2f pos = camera_screen_to_world(state->events.mouse_input.position, &state->main_camera, state->window.width, state->window.height);
        spawn_box(pos, vec4f_make(randf(), randf(), randf(), 0.4f));
    } 



    
    // Time slow down input.
    if (BIND_PRESSED(KEYBIND_TIME_TOGGLE)) {
        state->t.time_slow_factor++;
        state->t.delta_time_multi = 1.0f / (state->t.time_slow_factor % 5);
        
        console_log("Delta Time Multiplier: %f\n", state->t.delta_time_multi);
    }

    // Player logic.
    update_player(&state->player);
    
    // Lerp camera.
    state->main_camera.center = vec2f_lerp(state->main_camera.center, state->player.p_box.bound_box.center, 0.025f);

    // Boxes logic.
    update_boxes();

    // Cleaning destroyed phys_boxes. Following code might be moved to physics file later.
    for (s64 i = 0; i < array_list_length(&state->phys_boxes); i++) {
        // @Important: This is a check if a pointer is invalid, it is done by specifically looking at x dimension, since that is part of data that is set to 0.0f, phys box is no longer used, it's reference can be removed.
        if (fequal(state->phys_boxes[i]->bound_box.dimensions.x, 0.0f)) { 
            array_list_unordered_remove(&state->phys_boxes, i);
            i--;
            continue;
        }
    }

    // Physics update loop.
    phys_update(state->phys_boxes, array_list_length(&state->phys_boxes), &state->t);




    // Switching game state to menu.
    if (BIND_PRESSED(KEYBIND_MENU_ENTER)) {
        state->gs = MENU;
    }

}


float slider_val = 0.0f;
s32 slider_int = 0;
char text_input_buffer[100] = "";







void game_draw() {

    /**
     * -----------------------------------
     *  Drawing
     * -----------------------------------
     */


    // Get neccessary to draw resource: (Shaders, Fonts, Textures, etc.).
    Shader *quad_shader = hash_table_get(&state->shader_table, "quad");
    Shader *grid_shader = hash_table_get(&state->shader_table, "grid");
    Shader *line_shader = hash_table_get(&state->shader_table, "line");

    Font_Baked *font_medium = hash_table_get(&state->font_table, "medium");
    Font_Baked *font_small  = hash_table_get(&state->font_table, "small");


    // Load camera's projection into all shaders to correctly draw elements according to camera view.
    Matrix4f projection;

    projection = camera_calculate_projection(&state->main_camera, (float)state->window.width, (float)state->window.height);

    shader_update_projection(quad_shader, &projection);
    shader_update_projection(grid_shader, &projection);
    shader_update_projection(line_shader, &projection);


    // Reset viewport.
    viewport_reset(state->window.width, state->window.height);

    // Clear the screen with clear_color.
    glClearColor(state->clear_color.x, state->clear_color.y, state->clear_color.z, state->clear_color.w);
    glClear(GL_COLOR_BUFFER_BIT);


    // Grid drawing.
    draw_begin(&state->grid_drawer);

    Vec2f p0, p1;
    p0 = vec2f_make(-8.0f, -5.0f);
    p1 = vec2f_make(8.0f, 5.0f);
    // draw_quad(p0, p1, vec4f_make(0.8f, 0.8f, 0.8f, 0.6f), NULL, p0, p1, NULL, 0.0f, NULL);
    draw_grid(p0, p1, vec4f_make(0.8f, 0.8f, 0.8f, 0.6f), NULL);

    draw_end();



    // Regular quad drawing.
    draw_begin(&state->quad_drawer);

    draw_player(&state->player);
    draw_boxes();

    draw_end();




    // Line drawing.
    glLineWidth(2.0f);

    line_draw_begin(&state->line_drawer);

    draw_boxes_outline();

    draw_sword_line(&state->player.sword);


    // Test interpolation of angle matrix.
    Transform mat = transform_make_trs_2d(VEC2F_ORIGIN, deg2rad(30.0f), vec2f_make(1.0f, 2.0f));
    Transform reflection = transform_make_scale_2d(vec2f_make(-1.0f, 1.0f));
    mat = matrix4f_multiplication(&reflection, &mat);


    Vec2f right = matrix4f_mul_vec2f(mat, VEC2F_RIGHT);
    Vec2f up = matrix4f_mul_vec2f(mat, VEC2F_UP);
    Vec2f origin = matrix4f_mul_vec2f(mat, VEC2F_ORIGIN);

    draw_line(origin, right, VEC4F_RED, NULL);
    draw_line(origin, up, VEC4F_GREEN, NULL);




    line_draw_end();

    glLineWidth(1.0f);



    // Before drawing UI making final call to draw debug lines that are globally added to "debug_vert_buffer" throughout the update logic.
    vertex_buffer_draw_lines(&state->debug_vert_buffer, &state->line_drawer);
    vertex_buffer_clear(&state->debug_vert_buffer);
    


    // Get neccessary to draw resource: (Shaders, Fonts, Textures, etc.).
    state->ui.font = font_medium;

    Shader *ui_quad_shader = hash_table_get(&state->shader_table, "ui_quad");

    projection = screen_calculate_projection(state->window.width, state->window.height);
    shader_update_projection(ui_quad_shader, &projection);



    // Test UI drawing
    draw_begin(&state->ui_drawer);


    state->ui.x_axis = UI_ALIGN_OPPOSITE;
    state->ui.y_axis = UI_ALIGN_DEFAULT;
    UI_WINDOW(state->window.width, state->window.height,
        UI_FRAME(220, 120, 
            UI_FRAME(120, 120, 
                if (UI_BUTTON(vec2f_make(120, 80), "Spawn box")) {
                    spawn_box(VEC2F_ORIGIN, vec4f_make(randf(), randf(), randf(), 0.4f));
                }
                if (UI_BUTTON(vec2f_make(120, 40), "Pop box")) {
                    for (s32 i = MAX_BOXES - 1; i > -1; i--) {
                        if (!state->boxes[i].destroyed) {
                            state->boxes[i].p_box.bound_box.dimensions.x = 0.0f;
                            state->boxes[i].destroyed = true;
                            break;
                        }   
                    }
                }
            );
            
            ui_sameline();

            UI_FRAME(100, 120, 
                for (s32 i = 0; i < 1; i++) {
                    ui_set_prefix(i);
                    if (UI_BUTTON(vec2f_make(100, 40), "Menu")) {
                        state->gs = MENU;
                    }
                }
                UI_SLIDER_FLOAT(vec2f_make(100, 40), &slider_val, -0.0f, 1.0f);
                UI_SLIDER_INT(vec2f_make(100, 40), &slider_int, -1, 12942);
            );
            

            // UI_SLIDER_FLOAT(vec2f_make(220, 40), &slider_val, -2.0f, 2.0f);
            // UI_SLIDER_FLOAT(vec2f_make(220, 40), &slider_val, -4.0f, 4.0f);
        );
    );


    



    // Test infor display
    u32 info_buffer_size = 256;
    s32 written = 0;
    char info_buffer[info_buffer_size];

    written += snprintf(info_buffer + written, info_buffer_size - written, "FPS: %3.2f\n", 1 / state->t.delta_time);
    written += snprintf(info_buffer + written, info_buffer_size - written, "Phys Box Count: %u\n", array_list_length(&state->phys_boxes));
    written += snprintf(info_buffer + written, info_buffer_size - written, "Slider value: %2.2f\n", slider_val);
    written += snprintf(info_buffer + written, info_buffer_size - written, "Slider int value: %2d\n", slider_int);
    





    state->ui.x_axis = UI_ALIGN_DEFAULT;
    state->ui.y_axis = UI_ALIGN_OPPOSITE;

    UI_WINDOW(state->window.width, state->window.height,
        // if (UI_INPUT_FIELD(vec2f_make(500, 40), text_input_buffer, 100)) {
        //     printf("%s\n", text_input_buffer);
        //     text_input_buffer[0] = '\0';
        //     state->events.text_input.write_index = 0;
        //     state->events.text_input.length = 0;
        // }
        ui_text(info_buffer);
    );



    draw_end();
}







void menu_update() {

    // Switching game state back to play.
    if (BIND_PRESSED(KEYBIND_MENU_EXIT)) {
        state->gs = PLAY;
    }
}


bool toggle;

void menu_draw() {
    // Reset viewport.
    viewport_reset(state->window.width, state->window.height);

    // Clear screen.
    glClearColor(0.3f, 0.1f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Get neccessary to draw resource: (Shaders, Fonts, Textures, etc.).
    Font_Baked *font_medium = hash_table_get(&state->font_table, "medium");
    state->ui.font = font_medium;

    Shader *ui_quad_shader = hash_table_get(&state->shader_table, "ui_quad");
    Shader *line_shader = hash_table_get(&state->shader_table, "line");

    Matrix4f projection = screen_calculate_projection(state->window.width, state->window.height);
    shader_update_projection(ui_quad_shader, &projection);
    shader_update_projection(line_shader, &projection);


    
    draw_begin(&state->ui_drawer);

    state->ui.x_axis = UI_ALIGN_DEFAULT;
    state->ui.y_axis = UI_ALIGN_OPPOSITE;

    UI_WINDOW(state->window.width, state->window.height,
        UI_FRAME(state->window.width, 50,
            if (UI_BUTTON(vec2f_make(100, 50), "Go Back")) {
                state->gs = PLAY;
            }
        );
    );

    state->ui.y_axis = UI_ALIGN_DEFAULT;

    UI_WINDOW(state->window.width, state->window.height,
        ui_text("Menu");
    );

    draw_end();
    // // Pushing new origin.
    // array_list_append(&state->ui.origin_stack, VEC2F_ORIGIN);

    // ui_cursor_reset();

    // // Buttons.
    // if (UI_BUTTON(vec2f_make(100, 50), "Click me")) {
    //     toggle = !toggle;
    // }

    // if (toggle) {
    //     UI_BUTTON(vec2f_make(200, 100), "UI Buttons");
    //     if (UI_BUTTON(vec2f_make(200, 100), "GO BACK")) {
    //         state->gs = PLAY;
    //     }
    //     UI_BUTTON(vec2f_make(200, 100), "Centered text");
    // }

    // // ui_text("Menu");

    // array_list_pop(&state->ui.origin_stack);


    vertex_buffer_draw_lines(&state->debug_vert_buffer, &state->line_drawer);
    vertex_buffer_clear(&state->debug_vert_buffer);

}







#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 700

const char* APP_NAME = "Game in C";


void init(State *s) {
    state = s;



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
    state->window = create_gl_window(APP_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT);

    if (state->window.ptr == NULL) {
        fprintf(stderr, "%s Couldn't create window.\n", debug_error_str);
        exit(1);
    }


                        ;


    // Keyboard init.
    keyboard_state_init();

    // Initing graphics.
    graphics_init();




    // Initing events.
    init_events_handler(&state->events);

    // Setting random seed.
    srand((u32)time(NULL));


    // Debug verticies buffer allocation.
    state->debug_vert_buffer = vertex_buffer_make(); // @Leak



    /**
     * Setting hash tables for resources. 
     * Since they are dynamically allocated pointers will not change after hot reloading.
     */
    state->shader_table = hash_table_make(Shader, 8, &std_allocator);
    state->font_table = hash_table_make(Font_Baked, 8, &std_allocator);
    



    // Allocating space for phys boxes.
    state->phys_boxes = array_list_make(Phys_Box *, 8, &std_allocator);



    state->boxes = malloc(MAX_BOXES * sizeof(Box)); // @Leak.
    if (state->boxes == NULL)
        printf_err("Couldn't allocate boxes array.\n");
    
    // Small init of boxes for other algorithms to work properly.
    for (u32 i = 0; i < MAX_BOXES; i++) {
        state->boxes[i].destroyed = true;
    }

    // Set game state.
    s->gs = PLAY;



    spawn_rect(vec2f_make(-8.0f, -5.0f), vec2f_make(8.0f, -6.0f), vec4f_make(randf(), randf(), randf(), 0.4f));
    spawn_obstacle(vec2f_make(4.0f, -1.0f), 6.0f, 1.0f, vec4f_make(randf(), randf(), randf(), 0.4f), PI / 6);
    spawn_obstacle(vec2f_make(-4.0f, 1.5f), 6.0f, 1.0f, vec4f_make(randf(), randf(), randf(), 0.4f), -PI / 6);

    spawn_player(VEC2F_ORIGIN, vec4f_make(0.0f, 1.0f, 0.0f, 0.2f));
}

             ;

void load(State *s) {
    state = s;

    // Loading globals.
    load_globals();


    state->clear_color = vec4f_make(0.1f, 0.1f, 0.4f, 1.0f);

    // Main camera init.
    state->main_camera = camera_make(VEC2F_ORIGIN, 48);

    // Shader loading.
    Shader grid_shader = shader_load("res/shader/grid.glsl");
    shader_init_uniforms(&grid_shader);
    hash_table_put(&state->shader_table, grid_shader, "grid");

    Shader quad_shader = shader_load("res/shader/quad.glsl");
    shader_init_uniforms(&quad_shader);
    hash_table_put(&state->shader_table, quad_shader, "quad");

    Shader ui_quad_shader = shader_load("res/shader/ui_quad.glsl");
    shader_init_uniforms(&ui_quad_shader);
    hash_table_put(&state->shader_table, ui_quad_shader, "ui_quad");

    Shader line_shader = shader_load("res/shader/line.glsl");
    shader_init_uniforms(&line_shader);
    hash_table_put(&state->shader_table, line_shader, "line");

    // Drawers init.
    drawer_init(&state->quad_drawer, hash_table_get(&state->shader_table, "quad"));
    drawer_init(&state->grid_drawer, hash_table_get(&state->shader_table, "grid"));
    drawer_init(&state->ui_drawer, hash_table_get(&state->shader_table, "ui_quad"));
    line_drawer_init(&state->line_drawer, hash_table_get(&state->shader_table, "line"));



    // Font loading.
    u8* font_data = read_file_into_buffer("res/font/font.ttf", NULL, &std_allocator);

    Font_Baked font_baked_medium = font_bake(font_data, 20.0f);
    hash_table_put(&state->font_table, font_baked_medium, "medium");

    Font_Baked font_baked_small = font_bake(font_data, 16.0f);
    hash_table_put(&state->font_table, font_baked_small, "small");

    free(font_data);
    

    // Init console.
    init_console(state);

    // Init ui.
    ui_init(&state->ui, &state->events.mouse_input);

    // Player loading.
    state->player.speed = 5.0f;

}


static s32 load_counter = 5;

/**
 * @Important: In game update loops the order of procedures is: Updating -> Drawing.
 * Where in updating all logic of the game loop is contained including inputs, sound and so on.
 * Drawing part is only responsible for putting pixels accrodingly with calculated data in "Updating" part.
 */
void update(State *s) {
    state = s;
    
    // Handling events
    handle_events(&state->events, &state->window, &state->t);


    // A simple and easy way to prevent delta_time jumps when loading.
    if (load_counter == 0) {
        state->t.delta_time_multi = 1.0f / (state->t.time_slow_factor % 5);
    }
    if (load_counter > -1) {
        load_counter--;
    }



    // Switch to handle different game states.
    switch (state->gs) {
        case PLAY:
            game_update();
            game_draw();
            break;
        case MENU:
            menu_update();
            menu_draw();
            break;
        default:
            printf_err("Couldn't handle current game state value.\n");
    }

    // Console update.
    console_update(&state->window, &state->events, &state->t);
    // Console drawing.
    console_draw(&state->window);
   


    // Checking for gl error.
    check_gl_error();


    // Swap buffers to display the rendered image.
    SDL_GL_SwapWindow(state->window.ptr);

    
    // Checking for hot reload.
#ifdef DEV
    if (BIND_PRESSED(SDLK_r)) {
        state->should_hot_reload = true;
    }
#endif


    // Post updating input.
    keyboard_state_old_update();
}

void unload(State *s) {
    state->t.delta_time_multi = 0.0f;

    console_free();

    shader_unload(hash_table_get(&state->shader_table, "quad"));
    shader_unload(hash_table_get(&state->shader_table, "grid"));
    shader_unload(hash_table_get(&state->shader_table, "ui_quad"));
    shader_unload(hash_table_get(&state->shader_table, "line"));
    
    drawer_free(&state->quad_drawer);
    drawer_free(&state->grid_drawer);
    drawer_free(&state->ui_drawer);
    line_drawer_free(&state->line_drawer);
    
    font_free(hash_table_get(&state->font_table, "medium"));
    font_free(hash_table_get(&state->font_table, "small"));
}
















