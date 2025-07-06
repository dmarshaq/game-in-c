#include "game/plug.h"
#include "console.h"
#include "game/console.h"
#include "core/core.h"
#include "core/type.h"
#include "core/graphics.h"
#include "core/structs.h"
#include "core/file.h"
#include "core/mathf.h"
#include "core/input.h"

#include "game/event.h"
#include "game/draw.h"

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
static const Vec2f GRAVITY_ACCELERATION = (Vec2f){ 0.0f, -9.81f };
static const u8 MAX_BOXES = 32;
static const u8 MAX_IMPULSES = 16;
static const u8 MAX_PHYS_BOXES = 16;
static const u8 PHYS_ITERATIONS = 16;
static const float PHYS_ITERATION_STEP_TIME = (1.0f / PHYS_ITERATIONS);
static const float MAX_PLAYER_SPEED = 6.0f;




// Global state.
static Plug_State *state;







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
 * User Interface.
 */



// UI Origin logic.
Vec2f ui_current_frame_origin() {
    if (array_list_length(&state->ui.frame_stack) < 1) {
        printf_err("UI Current frame doesn't exist.\n");
        return VEC2F_ORIGIN;
    }
    return state->ui.frame_stack[array_list_length(&state->ui.frame_stack) - 1].origin;
}

Vec2f ui_current_frame_size() {
    if (array_list_length(&state->ui.frame_stack) < 1) {
        printf_err("UI Current frame doesn't exist.\n");
        return VEC2F_ORIGIN;
    }
    return state->ui.frame_stack[array_list_length(&state->ui.frame_stack) - 1].size;
}

void ui_push_frame(float x, float y, float width, float height) {
    array_list_append(&state->ui.frame_stack, ((UI_Frame){
            .origin = vec2f_make(x, y),
            .size = vec2f_make(width, height),
            }) );
}

void ui_pop_frame() {
    array_list_pop(&state->ui.frame_stack);
}



// UI Init
void ui_init() {
    state->ui.cursor = vec2f_make(0, 0);
    state->ui.frame_stack = array_list_make(UI_Frame, 8, &std_allocator); // Maybe replace with the custom defined arena allocator later. @Leak.

    state->ui.x_axis = UI_ALIGN_OPPOSITE;
    state->ui.y_axis = UI_ALIGN_OPPOSITE;

    state->ui.element_size = VEC2F_ORIGIN;
    state->ui.sameline = false;

    state->ui.active_line_id = -1;
    state->ui.active_prefix_id = -1;
    state->ui.set_prefix_id = -1;

    state->ui.theme = (UI_Theme) {
        .bg             = (Vec4f){ 0.12f, 0.12f, 0.14f, 1.0f },   // Dark slate background
        .light          = (Vec4f){ 0.85f, 0.85f, 0.88f, 1.0f },   // Soft light gray
        .btn_bg         = (Vec4f){ 0.12f, 0.16f, 0.28f, 1.0f },   // Deeper, richer base button
        .btn_bg_hover   = (Vec4f){ 0.20f, 0.26f, 0.40f, 1.0f },   // Gentle contrast on hover
        .btn_bg_press   = (Vec4f){ 0.08f, 0.10f, 0.22f, 1.0f },   // Subtle shadowy press state
        .text           = (Vec4f){ 0.97f, 0.96f, 0.92f, 1.0f },   // Warm white text
    };
}

// UI prefix id.
void ui_set_prefix(s32 id) {
    state->ui.set_prefix_id = id;
}

void ui_activate_next() {
    state->ui.activate_next = true;
}

void ui_end_element() {
    state->ui.set_prefix_id = -1;
    state->ui.activate_next = false;
}

// UI Alignment.
Vec2f ui_current_aligned_origin() {
    Vec2f origin = ui_current_frame_origin();
    Vec2f size = ui_current_frame_size();
    if (state->ui.x_axis > 0)
        origin.x += size.x;
    if (state->ui.y_axis > 0)
        origin.y += size.y;
    return origin;
}


// UI Cursor related logic.
void ui_cursor_reset() {
    state->ui.cursor = ui_current_aligned_origin();
    state->ui.element_size = VEC2F_ORIGIN;
    state->ui.sameline = false;
}

/**
 * Advances cursor based on "sameline", "element_size", and next element "size" values, according to alignment.
 */
void ui_cursor_advance(Vec2f size) {
    if (state->ui.sameline) {
        state->ui.cursor.y += state->ui.y_axis * (state->ui.line_height - size.y);
        state->ui.line_height = size.y > state->ui.line_height ? size.y : state->ui.line_height;
        state->ui.cursor.x += !state->ui.x_axis * state->ui.element_size.x - state->ui.x_axis * size.x;
        state->ui.sameline = false;
    } else {
        state->ui.line_height = size.y;
        state->ui.cursor.x = ui_current_aligned_origin().x - state->ui.x_axis * size.x;
        state->ui.cursor.y += !state->ui.y_axis * state->ui.element_size.y - state->ui.y_axis * size.y;
    }
}

void ui_set_element_size(Vec2f size) {
    state->ui.element_size = size;
}

void ui_sameline() {
    state->ui.sameline = true;
}









// UI Frame.
void ui_draw_rect(Vec2f position, Vec2f size, Vec4f color) {
    Vec2f p0 = position;
    Vec2f p1 = vec2f_sum(position, size);

    float quad_data[48] = {
        p0.x, p0.y, 0.0f, color.x, color.y, color.z, color.w, 0.0f, 0.0f, size.x, size.y, -1.0f,
        p1.x, p0.y, 0.0f, color.x, color.y, color.z, color.w, 1.0f, 0.0f, size.x, size.y, -1.0f,
        p0.x, p1.y, 0.0f, color.x, color.y, color.z, color.w, 0.0f, 1.0f, size.x, size.y, -1.0f,
        p1.x, p1.y, 0.0f, color.x, color.y, color.z, color.w, 1.0f, 1.0f, size.x, size.y, -1.0f,
    };
    
    draw_quad_data(quad_data, 1);
}


#define UI_FRAME(width, height, code)\
    ui_cursor_advance(vec2f_make(width, height));\
    ui_draw_rect(state->ui.cursor, vec2f_make(width, height), state->ui.theme.bg);\
    ui_push_frame(state->ui.cursor.x, state->ui.cursor.y, width, height);\
    ui_cursor_reset();\
    code\
    ui_cursor_reset();\
    ui_pop_frame();\
    ui_cursor_advance(vec2f_make(width, height));\
    ui_set_element_size(vec2f_make(width, height))

#define UI_WINDOW(code)\
    ui_push_frame(0, 0, state->window.width, state->window.height);\
    ui_cursor_reset();\
    code\
    ui_pop_frame()\

    







bool ui_is_hover(Vec2f size) {
    return value_inside_domain(state->ui.cursor.x, state->ui.cursor.x + size.x, state->events.mouse_input.position.x) && value_inside_domain(state->ui.cursor.y, state->ui.cursor.y + size.y, state->events.mouse_input.position.y);
}

void ui_draw_text(char *text, Vec2f position, Vec4f color) {
    u64 text_length = strlen(text);

    // Scale and adjust current_point.
    Vec2f current_point = position;
    Font_Baked *font = state->ui.font;
    float origin_x = current_point.x;
    current_point.y += (float)font->baseline;

    // Text rendering variables.
    s32 font_char_index;
    u16 width, height;
    stbtt_bakedchar *c;

    for (u64 i = 0; i < text_length; i++) {
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


            Vec2f p0 = vec2f_make(current_point.x + c->xoff, current_point.y - c->yoff - height);
            Vec2f p1 = vec2f_make(current_point.x + c->xoff + width, current_point.y - c->yoff);

            Vec2f uv0 = vec2f_make(c->x0 / (float)font->bitmap.width, c->y1 / (float)font->bitmap.height);
            Vec2f uv1 = vec2f_make(c->x1 / (float)font->bitmap.width, c->y0 / (float)font->bitmap.height);

            float mask_slot = add_texture_to_slots(&font->bitmap);              

            float quad_data[48] = {
                p0.x, p0.y, 0.0f, color.x, color.y, color.z, color.w, uv0.x, uv0.y, 1.0f, 1.0f, mask_slot,
                p1.x, p0.y, 0.0f, color.x, color.y, color.z, color.w, uv1.x, uv0.y, 1.0f, 1.0f, mask_slot,
                p0.x, p1.y, 0.0f, color.x, color.y, color.z, color.w, uv0.x, uv1.y, 1.0f, 1.0f, mask_slot,
                p1.x, p1.y, 0.0f, color.x, color.y, color.z, color.w, uv1.x, uv1.y, 1.0f, 1.0f, mask_slot,
            };

            draw_quad_data(quad_data, 1);


            current_point.x += font->chars[font_char_index].xadvance;
        }

    }
}

void ui_draw_text_centered(char *text, Vec2f position, Vec2f size, Vec4f color) {
    if (text == NULL)
        return;

    Vec2f t_size = text_size(CSTR(text), state->ui.font);

    // Following ui_draw_text centered calculations are for the system where y axis points up, and x axis to the right.
    ui_draw_text(text, vec2f_make(position.x + (size.x - t_size.x) * 0.5f, position.y + (size.y + t_size.y) * 0.5f), color);
}


// UI Button

#define UI_BUTTON(size, text) ui_button(size, text, __LINE__)

bool ui_button(Vec2f size, char *text, s32 id) {
    ui_cursor_advance(size);
    ui_set_element_size(size);

    s32 prefix_id = state->ui.set_prefix_id;

    Vec4f res_color = state->ui.theme.btn_bg;
    bool  res_ret   = false;
    

    if (ui_is_hover(size) || state->ui.activate_next) {
        if (state->ui.active_line_id == id && state->ui.active_prefix_id == prefix_id) {
            if (state->events.mouse_input.left_unpressed) {
                state->ui.active_line_id = -1;
                state->ui.active_prefix_id = -1;

                res_color = state->ui.theme.btn_bg_hover;
                res_ret   = true;
                goto leave_draw;
            }

            res_color = state->ui.theme.btn_bg_press;
            goto leave_draw;
        }

        if (state->events.mouse_input.left_pressed || state->ui.activate_next) {
            state->ui.active_line_id = id;
            state->ui.active_prefix_id = prefix_id;
        }

        res_color = state->ui.theme.btn_bg_hover;

    } else if (state->ui.active_line_id == id && state->ui.active_prefix_id == prefix_id) {
        state->ui.active_line_id = -1;
        state->ui.active_prefix_id = -1;
    } 

leave_draw:
    
    ui_draw_rect(state->ui.cursor, size, res_color);
    ui_draw_text_centered(text, state->ui.cursor, size, state->ui.theme.text);

    ui_end_element();

    return res_ret;
}


// UI Slider


#define UI_SLIDER_INT(size, output, min, max) ui_slider_int(size, output, min, max, __LINE__)

bool ui_slider_int(Vec2f size, s32 *output, s32 min, s32 max, s32 id) {
    ui_cursor_advance(size);
    ui_set_element_size(size);

    s32 prefix_id = state->ui.set_prefix_id;

    float scale = (max - min) / size.x;

    Vec4f res_color = state->ui.theme.btn_bg;
    Vec4f res_knob_color = state->ui.theme.btn_bg_hover;
    bool  res_ret   = false;
    
    if (state->ui.active_line_id == id && state->ui.active_prefix_id == prefix_id) {
        if (state->events.mouse_input.left_hold) {
            // If was pressed, and holding.
            float a = state->events.mouse_input.position.x * scale - (state->ui.cursor.x * scale - min);
            *output = (s32)clamp(a, min, max);

            res_color = state->ui.theme.btn_bg_press;
            res_knob_color = state->ui.theme.text;
            res_ret   = true;
            goto leave_draw;
        } else {
            // If was pressed, but not holding anymore.
            state->ui.active_line_id = -1;
            state->ui.active_prefix_id = -1;
            goto leave_draw;
        }
    }
    
    if (ui_is_hover(size)) {
        // If just pressed.
        if (state->events.mouse_input.left_pressed) {
            state->ui.active_line_id = id;
            state->ui.active_prefix_id = prefix_id;
        }

        res_color = state->ui.theme.btn_bg_hover;
    } 


leave_draw:

    ui_draw_rect(vec2f_make(state->ui.cursor.x, state->ui.cursor.y + size.y * 0.5f - 5), vec2f_make(size.x, 10), res_color); // Slider line.
    ui_draw_rect(vec2f_make(state->ui.cursor.x + (*output - min) / scale - 8, state->ui.cursor.y + size.y * 0.5f - 8), vec2f_make(16, 16), res_knob_color); // Slider knob.

    ui_end_element();

    return res_ret;
}

#define UI_SLIDER_FLOAT(size, output, min, max) ui_slider_float(size, output, min, max, __LINE__)

bool ui_slider_float(Vec2f size, float *output, float min, float max, s32 id) {
    ui_cursor_advance(size);
    ui_set_element_size(size);

    s32 prefix_id = state->ui.set_prefix_id;

    float scale = (max - min) / size.x;

    Vec4f res_color = state->ui.theme.btn_bg;
    Vec4f res_knob_color = state->ui.theme.btn_bg_hover;
    bool  res_ret   = false;
    
    if (state->ui.active_line_id == id && state->ui.active_prefix_id == prefix_id) {
        if (state->events.mouse_input.left_hold) {
            // If was pressed, and holding.
            float a = state->events.mouse_input.position.x * scale - (state->ui.cursor.x * scale - min);
            *output = clamp(a, min, max);

            res_color = state->ui.theme.btn_bg_press;
            res_knob_color = state->ui.theme.text;
            res_ret   = true;
            goto leave_draw;
        } else {
            // If was pressed, but not holding anymore.
            state->ui.active_line_id = -1;
            state->ui.active_prefix_id = -1;
            goto leave_draw;
        }
    }
    
    if (ui_is_hover(size)) {
        // If just pressed.
        if (state->events.mouse_input.left_pressed) {
            state->ui.active_line_id = id;
            state->ui.active_prefix_id = prefix_id;
        }

        res_color = state->ui.theme.btn_bg_hover;
    } 


leave_draw:

    ui_draw_rect(vec2f_make(state->ui.cursor.x, state->ui.cursor.y + size.y * 0.5f - 5), vec2f_make(size.x, 10), res_color); // Slider line.
    ui_draw_rect(vec2f_make(state->ui.cursor.x + (*output - min) / scale - 8, state->ui.cursor.y + size.y * 0.5f - 8), vec2f_make(16, 16), res_knob_color); // Slider knob.
    // ui_draw_rect(vec2f_make(state->ui.cursor.x + (*output - min) / scale - 5, state->ui.cursor.y), vec2f_make(10, size.y), res_knob_color); // Slider knob bar.

    ui_end_element();

    return res_ret;
}


// UI Input field

#define UI_INPUT_FIELD(size, buffer, buffer_size) ui_input_field(size, buffer, buffer_size, __LINE__)

bool ui_input_field(Vec2f size, char *buffer, s32 buffer_size, s32 id) {
    // Mouse_Input *mouse_i = &state->events.mouse_input;
    // Text_Input *text_i = &state->events.text_input;

    // ui_cursor_advance(size);
    // ui_set_element_size(size);

    // s32 prefix_id = state->ui.set_prefix_id;

    // Vec4f res_color = state->ui.theme.btn_bg;
    // Vec4f res_bar_color = state->ui.theme.btn_bg_hover;
    // bool  res_flushed = false;

    // if (state->ui.active_line_id == id && state->ui.active_prefix_id == prefix_id) {
    //     if (mouse_i->left_pressed || pressed(SDLK_ESCAPE)) {
    //         // If was pressed but was pressed again outside (deselected)
    //         state->ui.active_line_id = -1;
    //         state->ui.active_prefix_id = -1;

    //         SDL_StopTextInput();

    //         text_i->buffer = NULL;
    //         text_i->capacity = 0;
    //         text_i->length = 0;
    //         text_i->write_index = 0;


    //         goto leave_draw;
    //     }

    //     res_color = state->ui.theme.btn_bg_press;

    //     if (pressed(SDLK_RETURN)) {
    //         res_flushed = true;
    //         goto leave_draw;
    //     } 


    //     // If is still active.
    //     


    // }



    // if (ui_is_hover(size) || state->ui.activate_next) {
    //     // If just pressed.
    //     if (mouse_i->left_pressed || state->ui.activate_next) {
    //         state->ui.active_line_id = id;
    //         state->ui.active_prefix_id = prefix_id;
    //         
    //         SDL_StartTextInput();

    //         // Setting text input for appending.
    //         text_i->buffer = buffer;
    //         text_i->capacity = buffer_size;
    //         text_i->length = strlen(buffer);
    //         text_i->write_index = text_i->length;
    //         
    //     }

    //     res_color = state->ui.theme.btn_bg_hover;

    //     goto leave_draw;
    // }



//leav// e_draw:
    //  Vec2f t_size = text_size(CSTR(buffer), state->ui.font);
    //  ui_draw_rect(state->ui.cursor, size, res_color); // Input field.
    // // Please rework
    //  // ui_draw_rect(vec2f_make(state->ui.cursor.x + text_size(CSTR(buffer), text_i->write_index, state->ui.font).x, state->ui.cursor.y), vec2f_make(10, size.y), res_bar_color); // Input bar.
    //  ui_draw_text(buffer, vec2f_make(state->ui.cursor.x, state->ui.cursor.y + (size.y + t_size.y) * 0.5f), state->ui.theme.text);

    //  ui_end_element();

    //  return res_flushed;
    return false;
}




void ui_text(char *text) {
    Vec2f t_size = text_size(CSTR(text), state->ui.font);
    ui_cursor_advance(t_size);
    ui_set_element_size(t_size);
    ui_draw_text(text, vec2f_make(state->ui.cursor.x, state->ui.cursor.y + t_size.y), state->ui.theme.text);
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
 * Physics.
 */

/**
 * Standard units used:
 *      Mass -> kg
 *      Speed -> m/s
 *      Acceleration -> m/s^2
 *      Force -> m/s^2 * kg -> N
 *
 * Important formulas:
 *  
 *  Instanteneous acceleration and force.
 *      Vi + a = Vf
 *      Vi + (F / m) = Vf
 *  
 *  Acceleration and force over period of time / continuous.
 *      Vi + a * dt = Vf
 *      Vi + (F / m) * dt = Vf
 */

/**
 * Applies instanteneous force to rigid body.
 */
void phys_apply_force(Body_2D *body, Vec2f force) {
    body->velocity = vec2f_sum(body->velocity, vec2f_multi_constant(force, body->inv_mass));
}

/**
 * Applies instanteneous acceleration to rigid body.
 */
void phys_apply_acceleration(Body_2D *body, Vec2f acceleration) {
    body->velocity = vec2f_sum(body->velocity, acceleration);
}

void phys_apply_angular_acceleration(Body_2D *body, float acceleration) {
    body->angular_velocity += acceleration;
}

void phys_init() {
    // Setting physics variables.
    state->impulses = array_list_make(Impulse, MAX_IMPULSES, &std_allocator); // @Leak.
    state->phys_boxes = array_list_make(Phys_Box *, MAX_PHYS_BOXES, &std_allocator); // @Leak.

}

void phys_add_impulse(Vec2f force, u32 milliseconds, Body_2D *body) {
    Impulse impulse = (Impulse) { 
        .delta_force = vec2f_divide_constant(force, (float)milliseconds), 
        .milliseconds = milliseconds, 
        .target = body 
    };
    array_list_append(&state->impulses, impulse);
}

void phys_apply_impulses() {
    for (u32 i = 0; i < array_list_length(&state->impulses); i++) {
        u32 force_time_multi = state->t.delta_time_milliseconds;

        if (state->impulses[i].milliseconds < state->t.delta_time_milliseconds) {
            force_time_multi = state->impulses[i].milliseconds;
            state->impulses[i].milliseconds = 0;
        }
        else {
            state->impulses[i].milliseconds -= state->t.delta_time_milliseconds;
        }

        state->impulses[i].target->velocity = vec2f_sum(state->impulses[i].target->velocity, vec2f_multi_constant(vec2f_divide_constant(state->impulses[i].delta_force, state->impulses[i].target->mass), (float)force_time_multi));

        if (state->impulses[i].milliseconds == 0) {
            array_list_unordered_remove(&state->impulses, i);
            i--;
        }
    }
}

/**
 * Internal function.
 * @Important: "axis1" should always correspond to the axis alligned with "obb1".
 * Returns min separation between "obb1" and "obb2" projected points on "axis1".
 */
float phys_sat_min_depth_on_normal(OBB *obb1, Vec2f axis1, OBB *obb2) {
    // Getting minA and maxA out of 2 points.
    float minA = vec2f_dot(axis1, obb_p0(obb1));
    float maxA = vec2f_dot(axis1, obb_p1(obb1));

    if (minA > maxA) {
        minA = maxA;
        maxA = vec2f_dot(axis1, obb_p0(obb1));
    }

    // Getting minB and maxB out of 4 points. @Optimization: Maybe find a way to calculate dot products only once and reuse them to find min, max.
    float minB = fminf(fminf(vec2f_dot(axis1, obb_p0(obb2)), vec2f_dot(axis1, obb_p1(obb2))), fminf(vec2f_dot(axis1, obb_p2(obb2)), vec2f_dot(axis1, obb_p3(obb2))));
    float maxB = fmaxf(fmaxf(vec2f_dot(axis1, obb_p0(obb2)), vec2f_dot(axis1, obb_p1(obb2))), fmaxf(vec2f_dot(axis1, obb_p2(obb2)), vec2f_dot(axis1, obb_p3(obb2))));
    
    float dist1 = maxB - minA; 
    float dist2 = maxA - minB; 

    return fabsf(fminf(dist1, dist2)) * sig(dist1 * dist2);
}

/**
 * Returns true of "obb1" and "obb2" touch.
 * Usefull for triggers.
 */
bool phys_sat_check_collision_obb(OBB *obb1, OBB *obb2) {
    return phys_sat_min_depth_on_normal(obb1, obb_right(obb1), obb2) > 0.0f &&
        phys_sat_min_depth_on_normal(obb1, obb_up(obb1), obb2) > 0.0f &&
        phys_sat_min_depth_on_normal(obb2, obb_right(obb2), obb1) > 0.0f &&
        phys_sat_min_depth_on_normal(obb2, obb_up(obb2), obb1) > 0.0f;
}

void phys_sat_find_min_depth_normal(OBB *obb1, OBB *obb2, float *depth, Vec2f *normal) {
    Vec2f normals[4] = {
        obb_right(obb1),
        obb_up(obb1),
        obb_right(obb2),
        obb_up(obb2)
    };
    float depths[4] = {
        phys_sat_min_depth_on_normal(obb1, normals[0], obb2),
        phys_sat_min_depth_on_normal(obb1, normals[1], obb2),
        phys_sat_min_depth_on_normal(obb2, normals[2], obb1),
        phys_sat_min_depth_on_normal(obb2, normals[3], obb1)
    };

    for (u32 i = 1; i < 4; i++) {
        if (depths[i] < depths[0]) {
            depths[0] = depths[i];
            normals[0] = normals[i];
        }
    }


    // Flip normal if it's not looking in direction of collosion.
    if (vec2f_dot(normals[0], vec2f_normalize(vec2f_difference(obb2->center, obb1->center))) < 0.0f) {
        normals[0] = vec2f_negate(normals[0]);
    }

    *depth = depths[0];
    *normal = normals[0];
}

/**
 * Sets both obbs apart based on depth and normal.
 * Usefull for two colliding dynamic objects.
 */
void phys_resolve_dynamic_obb_collision(OBB *obb1, OBB *obb2, float depth, Vec2f normal) {

    Vec2f displacement = vec2f_multi_constant(normal, depth / 2);

    obb1->center = vec2f_sum(obb1->center, vec2f_negate(displacement));
    obb2->center = vec2f_sum(obb2->center, displacement);
}

void phys_resolve_dynamic_body_collision_basic(Body_2D *body1, Body_2D *body2, Vec2f normal) {
    Vec2f relative_velocity = vec2f_difference(body2->velocity, body1->velocity);

    float e = fminf(body1->restitution, body2->restitution);

    float j = -(1.0f + e) * vec2f_dot(relative_velocity, normal);
    j /= (1.0f * body1->inv_mass + 1.0f * body2->inv_mass);

    phys_apply_force(body1, vec2f_multi_constant(normal, -j));
    phys_apply_force(body2, vec2f_multi_constant(normal, j));
}

void phys_resolve_dynamic_body_collision(Body_2D *body1, Body_2D *body2, Vec2f normal, Vec2f *contacts, u32 contacts_count) {
    // Calculated variables, used in both loops.
    Vec2f impulses[2] = { VEC2F_ORIGIN, VEC2F_ORIGIN };
    Vec2f r1_array[2] = { VEC2F_ORIGIN, VEC2F_ORIGIN };
    Vec2f r2_array[2] = { VEC2F_ORIGIN, VEC2F_ORIGIN };
    
    // Calculation variables.
    float e = fminf(body1->restitution, body2->restitution);

    Vec2f angular_lin_velocity1, angular_lin_velocity2, relative_velocity;
    float r1_perp_dot_n, r2_perp_dot_n, contact_velocity_mag, j;

    // Calculating impulses and vectors to contact points, for each contact point.
    for (u32 i = 0; i < contacts_count; i++) {
        r1_array[i] = vec2f_difference(contacts[i], body1->mass_center);
        r2_array[i] = vec2f_difference(contacts[i], body2->mass_center);

        angular_lin_velocity1 = vec2f_multi_constant(vec2f_make(-r1_array[i].y, r1_array[i].x), body1->angular_velocity);
        angular_lin_velocity2 = vec2f_multi_constant(vec2f_make(-r2_array[i].y, r2_array[i].x), body2->angular_velocity);

        relative_velocity = vec2f_difference(vec2f_sum(body2->velocity, angular_lin_velocity2), vec2f_sum(body1->velocity, angular_lin_velocity1));
        
        contact_velocity_mag = vec2f_dot(relative_velocity, normal);

        if (contact_velocity_mag > 0.0f) {
            continue;
        }

        r1_perp_dot_n = vec2f_cross(r1_array[i], normal);
        r2_perp_dot_n = vec2f_cross(r2_array[i], normal);

        j = -(1.0f + e) * contact_velocity_mag;
        j /= body1->inv_mass + body2->inv_mass + (r1_perp_dot_n * r1_perp_dot_n) * body1->inv_inertia + (r2_perp_dot_n * r2_perp_dot_n) * body2->inv_inertia;
        j /= (float)contacts_count;

        impulses[i] = vec2f_multi_constant(normal, j);
    }

    // For each contact point applying impulse and angular acceleration.
    for (u32 i = 0; i < contacts_count; i++) {
        phys_apply_force(body1, vec2f_negate(impulses[i]));
        phys_apply_angular_acceleration(body1, -vec2f_cross(r1_array[i], impulses[i]) * body1->inv_inertia);

        phys_apply_force(body2, impulses[i]);
        phys_apply_angular_acceleration(body2, vec2f_cross(r2_array[i], impulses[i]) * body2->inv_inertia);
    }
}

typedef struct phys_collision_calc_vars {
    // Calculated variables, used in both loops.
    Vec2f impulse;
    Vec2f r1;
    Vec2f r2;
    
    // Calculation variables.
    float e;
    float sf;
    float df;

    Vec2f angular_lin_velocity1;
    Vec2f angular_lin_velocity2;
    Vec2f relative_velocity;
    float r1_perp_dot_n; 
    float r2_perp_dot_n;
    float contact_velocity_mag;
    float j[2];
    float jt;

    // Result variables.
    Vec2f res_velocity1;
    Vec2f res_velocity2;
    float res_angular_acceleration1;
    float res_angular_acceleration2;

} Phys_Collision_Calc_Vars;

static Phys_Collision_Calc_Vars p_calc_vars;

void phys_resolve_phys_box_collision_with_rotation_friction(Phys_Box *box1, Phys_Box *box2, Vec2f normal, Vec2f *contacts, u32 contacts_count) {
    // Setting up variables that can be calculated without contact points.
    p_calc_vars.e  = fminf(box1->body.restitution, box2->body.restitution);
    p_calc_vars.sf = (box1->body.static_friction + box2->body.static_friction) / 2;
    p_calc_vars.df = (box1->body.dynamic_friction + box2->body.dynamic_friction) / 2;

    // Init resulat vars to default values.
    p_calc_vars.res_velocity1 = VEC2F_ORIGIN;
    p_calc_vars.res_velocity2 = VEC2F_ORIGIN;
    p_calc_vars.res_angular_acceleration1 = 0.0f;
    p_calc_vars.res_angular_acceleration2 = 0.0f;


    p_calc_vars.r1 = VEC2F_ORIGIN;
    p_calc_vars.r2 = VEC2F_ORIGIN;
    p_calc_vars.impulse = VEC2F_ORIGIN;

    p_calc_vars.j[0] = 0.0f;
    p_calc_vars.j[1] = 0.0f;

    // Calculating impulses and vectors to contact points, for each contact point.
    for (u32 i = 0; i < contacts_count; i++) {
        p_calc_vars.r1 = vec2f_difference(contacts[i], box1->body.mass_center);
        p_calc_vars.r2 = vec2f_difference(contacts[i], box2->body.mass_center);

        p_calc_vars.angular_lin_velocity1 = vec2f_multi_constant(vec2f_make(-p_calc_vars.r1.y, p_calc_vars.r1.x), box1->body.angular_velocity);
        p_calc_vars.angular_lin_velocity2 = vec2f_multi_constant(vec2f_make(-p_calc_vars.r2.y, p_calc_vars.r2.x), box2->body.angular_velocity);

        p_calc_vars.relative_velocity = vec2f_difference(vec2f_sum(box2->body.velocity, p_calc_vars.angular_lin_velocity2), vec2f_sum(box1->body.velocity, p_calc_vars.angular_lin_velocity1));

        p_calc_vars.contact_velocity_mag = vec2f_dot(p_calc_vars.relative_velocity, normal);


        if (p_calc_vars.contact_velocity_mag > 0.0f) {
            continue;
        }

        p_calc_vars.r1_perp_dot_n = vec2f_cross(p_calc_vars.r1, normal);
        p_calc_vars.r2_perp_dot_n = vec2f_cross(p_calc_vars.r2, normal);

        p_calc_vars.j[i] = -(1.0f + p_calc_vars.e) * p_calc_vars.contact_velocity_mag;
        p_calc_vars.j[i] /= box1->body.inv_mass + box2->body.inv_mass + (p_calc_vars.r1_perp_dot_n * p_calc_vars.r1_perp_dot_n) * box1->body.inv_inertia + (p_calc_vars.r2_perp_dot_n * p_calc_vars.r2_perp_dot_n) * box2->body.inv_inertia;
        p_calc_vars.j[i] /= (float)contacts_count;



        p_calc_vars.impulse = vec2f_multi_constant(normal, p_calc_vars.j[i]);

        p_calc_vars.res_velocity1 = vec2f_sum(p_calc_vars.res_velocity1, vec2f_multi_constant(vec2f_negate(p_calc_vars.impulse), box1->body.inv_mass));
        p_calc_vars.res_angular_acceleration1 += -vec2f_cross(p_calc_vars.r1, p_calc_vars.impulse) * box1->body.inv_inertia;

        p_calc_vars.res_velocity2 = vec2f_sum(p_calc_vars.res_velocity2, vec2f_multi_constant(p_calc_vars.impulse, box2->body.inv_mass));
        p_calc_vars.res_angular_acceleration2 += vec2f_cross(p_calc_vars.r2, p_calc_vars.impulse) * box2->body.inv_inertia;
    }

    box1->body.velocity = vec2f_sum(box1->body.velocity, p_calc_vars.res_velocity1);
    if (box1->rotatable)
        box1->body.angular_velocity += p_calc_vars.res_angular_acceleration1;

    box2->body.velocity = vec2f_sum(box2->body.velocity, p_calc_vars.res_velocity2);
    if (box2->rotatable)
        box2->body.angular_velocity += p_calc_vars.res_angular_acceleration2;



    p_calc_vars.res_velocity1 = VEC2F_ORIGIN;
    p_calc_vars.res_velocity2 = VEC2F_ORIGIN;
    p_calc_vars.res_angular_acceleration1 = 0.0f;
    p_calc_vars.res_angular_acceleration2 = 0.0f;


    p_calc_vars.r1 = VEC2F_ORIGIN;
    p_calc_vars.r2 = VEC2F_ORIGIN;
    p_calc_vars.impulse = VEC2F_ORIGIN;


    // Friction.
    for (u32 i = 0; i < contacts_count; i++) {
        p_calc_vars.r1 = vec2f_difference(contacts[i], box1->body.mass_center);
        p_calc_vars.r2 = vec2f_difference(contacts[i], box2->body.mass_center);

        p_calc_vars.angular_lin_velocity1 = vec2f_multi_constant(vec2f_make(-p_calc_vars.r1.y, p_calc_vars.r1.x), box1->body.angular_velocity);
        p_calc_vars.angular_lin_velocity2 = vec2f_multi_constant(vec2f_make(-p_calc_vars.r2.y, p_calc_vars.r2.x), box2->body.angular_velocity);

        p_calc_vars.relative_velocity = vec2f_difference(vec2f_sum(box2->body.velocity, p_calc_vars.angular_lin_velocity2), vec2f_sum(box1->body.velocity, p_calc_vars.angular_lin_velocity1));

        Vec2f tanget = vec2f_difference(p_calc_vars.relative_velocity, vec2f_multi_constant(normal, vec2f_dot(p_calc_vars.relative_velocity, normal)));

        
        if (fequal(tanget.x, 0.0f) && fequal(tanget.y, 0.0f)) {
            continue;
        }

        tanget = vec2f_normalize(tanget);

        p_calc_vars.r1_perp_dot_n = vec2f_cross(p_calc_vars.r1, tanget);
        p_calc_vars.r2_perp_dot_n = vec2f_cross(p_calc_vars.r2, tanget);

        p_calc_vars.jt = -vec2f_dot(p_calc_vars.relative_velocity, tanget);
        p_calc_vars.jt /= box1->body.inv_mass + box2->body.inv_mass + (p_calc_vars.r1_perp_dot_n * p_calc_vars.r1_perp_dot_n) * box1->body.inv_inertia + (p_calc_vars.r2_perp_dot_n * p_calc_vars.r2_perp_dot_n) * box2->body.inv_inertia;
        p_calc_vars.jt /= (float)contacts_count;

        // Collumbs law.
        if (fabsf(p_calc_vars.jt) <= p_calc_vars.j[i] * p_calc_vars.sf) {
            // friction_impulses[i] = vec2f_multi_constant(tanget, p_calc_vars.jt);
            p_calc_vars.impulse = vec2f_multi_constant(tanget, p_calc_vars.jt);
        }
        else {
            // friction_impulses[i] = vec2f_multi_constant(tanget, -j_array[i] * p_calc_vars.df);
            p_calc_vars.impulse = vec2f_multi_constant(tanget, -p_calc_vars.j[i] * p_calc_vars.df);
        }

        p_calc_vars.res_velocity1 = vec2f_sum(p_calc_vars.res_velocity1, vec2f_multi_constant(vec2f_negate(p_calc_vars.impulse), box1->body.inv_mass));
        p_calc_vars.res_angular_acceleration1 += -vec2f_cross(p_calc_vars.r1, p_calc_vars.impulse) * box1->body.inv_inertia;

        p_calc_vars.res_velocity2 = vec2f_sum(p_calc_vars.res_velocity2, vec2f_multi_constant(p_calc_vars.impulse, box2->body.inv_mass));
        p_calc_vars.res_angular_acceleration2 += vec2f_cross(p_calc_vars.r2, p_calc_vars.impulse) * box2->body.inv_inertia;
    }

    box1->body.velocity = vec2f_sum(box1->body.velocity, p_calc_vars.res_velocity1);
    if (box1->rotatable)
        box1->body.angular_velocity += p_calc_vars.res_angular_acceleration1;

    box2->body.velocity = vec2f_sum(box2->body.velocity, p_calc_vars.res_velocity2);
    if (box2->rotatable)
        box2->body.angular_velocity += p_calc_vars.res_angular_acceleration2;
}

void phys_resolve_dynamic_body_collision_with_friction(Body_2D *body1, Body_2D *body2, Vec2f normal, Vec2f *contacts, u32 contacts_count) {
    // Calculated variables, used in both loops.
    Vec2f impulses[2] = { VEC2F_ORIGIN, VEC2F_ORIGIN };
    Vec2f r1_array[2] = { VEC2F_ORIGIN, VEC2F_ORIGIN };
    Vec2f r2_array[2] = { VEC2F_ORIGIN, VEC2F_ORIGIN };
    float j_array[2] = { 0.0f, 0.0f };
    Vec2f friction_impulses[2] = { VEC2F_ORIGIN, VEC2F_ORIGIN };
    
    // Calculation variables.
    float e = fminf(body1->restitution, body2->restitution);
    float sf = (body1->static_friction + body2->static_friction) / 2;
    float df = (body1->dynamic_friction + body2->dynamic_friction) / 2;

    Vec2f angular_lin_velocity1, angular_lin_velocity2, relative_velocity;
    float r1_perp_dot_n, r2_perp_dot_n, contact_velocity_mag, j, jt;

    // Calculating impulses and vectors to contact points, for each contact point.
    for (u32 i = 0; i < contacts_count; i++) {
        r1_array[i] = vec2f_difference(contacts[i], body1->mass_center);
        r2_array[i] = vec2f_difference(contacts[i], body2->mass_center);

        angular_lin_velocity1 = vec2f_multi_constant(vec2f_make(-r1_array[i].y, r1_array[i].x), body1->angular_velocity);
        angular_lin_velocity2 = vec2f_multi_constant(vec2f_make(-r2_array[i].y, r2_array[i].x), body2->angular_velocity);

        relative_velocity = vec2f_difference(vec2f_sum(body2->velocity, angular_lin_velocity2), vec2f_sum(body1->velocity, angular_lin_velocity1));

        contact_velocity_mag = vec2f_dot(relative_velocity, normal);


        if (contact_velocity_mag > 0.0f) {
            continue;
        }

        r1_perp_dot_n = vec2f_cross(r1_array[i], normal);
        r2_perp_dot_n = vec2f_cross(r2_array[i], normal);

        j = -(1.0f + e) * contact_velocity_mag;
        j /= body1->inv_mass + body2->inv_mass + (r1_perp_dot_n * r1_perp_dot_n) * body1->inv_inertia + (r2_perp_dot_n * r2_perp_dot_n) * body2->inv_inertia;
        j /= (float)contacts_count;


        j_array[i] = j;

        impulses[i] = vec2f_multi_constant(normal, j);
    }

    // For each contact point applying impulse and angular acceleration.
    for (u32 i = 0; i < contacts_count; i++) {
        phys_apply_force(body1, vec2f_negate(impulses[i]));
        phys_apply_angular_acceleration(body1, -vec2f_cross(r1_array[i], impulses[i]) * body1->inv_inertia);

        phys_apply_force(body2, impulses[i]);
        phys_apply_angular_acceleration(body2, vec2f_cross(r2_array[i], impulses[i]) * body2->inv_inertia);
    }

    // Friction.
    for (u32 i = 0; i < contacts_count; i++) {
        r1_array[i] = vec2f_difference(contacts[i], body1->mass_center);
        r2_array[i] = vec2f_difference(contacts[i], body2->mass_center);

        angular_lin_velocity1 = vec2f_multi_constant(vec2f_make(-r1_array[i].y, r1_array[i].x), body1->angular_velocity);
        angular_lin_velocity2 = vec2f_multi_constant(vec2f_make(-r2_array[i].y, r2_array[i].x), body2->angular_velocity);

        relative_velocity = vec2f_difference(vec2f_sum(body2->velocity, angular_lin_velocity2), vec2f_sum(body1->velocity, angular_lin_velocity1));


        Vec2f tanget = vec2f_difference(relative_velocity, vec2f_multi_constant(normal, vec2f_dot(relative_velocity, normal)));

        
        if (fequal(tanget.x, 0.0f) && fequal(tanget.y, 0.0f)) {
            continue;
        }

        tanget = vec2f_normalize(tanget);

        r1_perp_dot_n = vec2f_cross(r1_array[i], tanget);
        r2_perp_dot_n = vec2f_cross(r2_array[i], tanget);

        jt = -vec2f_dot(relative_velocity, tanget);
        jt /= body1->inv_mass + body2->inv_mass + (r1_perp_dot_n * r1_perp_dot_n) * body1->inv_inertia + (r2_perp_dot_n * r2_perp_dot_n) * body2->inv_inertia;
        jt /= (float)contacts_count;
    

        // Collumbs law.
        if (fabsf(jt) <= j * sf) {
            friction_impulses[i] = vec2f_multi_constant(tanget, jt);
        }
        else {
            friction_impulses[i] = vec2f_multi_constant(tanget, -j_array[i] * df);
        }
    }

    // For each contact point applying friction impulse and angular acceleration.
    for (u32 i = 0; i < contacts_count; i++) {
        phys_apply_force(body1, vec2f_negate(friction_impulses[i]));
        phys_apply_angular_acceleration(body1, -vec2f_cross(r1_array[i], friction_impulses[i]) * body1->inv_inertia);

        phys_apply_force(body2, friction_impulses[i]);
        phys_apply_angular_acceleration(body2, vec2f_cross(r2_array[i], friction_impulses[i]) * body2->inv_inertia);
    }
}

/**
 * Sets dynamic obb apart based on depth and normal.
 * Usefull for colliding dynamic object with immovable or static object.
 */
void phys_resolve_static_obb_collision(OBB *obb, float depth, Vec2f normal) {
    obb->center = vec2f_sum(obb->center, vec2f_multi_constant(normal, depth));
}

/**
 * Finds all contact points, maximum of 2, and stores them in "points" array.
 * @Important: "points" should not be NULL and should be of size two.
 * Returns count of points found.
 */
u32 phys_find_contanct_points_obb(OBB* obb1, OBB* obb2, Vec2f *points) {
    Vec2f verticies[8] = {
        obb_p0(obb1),
        obb_p2(obb1),
        obb_p1(obb1),
        obb_p3(obb1),
        obb_p0(obb2),
        obb_p2(obb2),
        obb_p1(obb2),
        obb_p3(obb2),
    };
    
    Vec2f a, b, p;
    float min_dist1 = FLT_MAX;
    float dist = 0.0f;
    u32 count = 0;

    for (u32 o = 0; o < 2; o++) {
        for (u32 i = 0; i < 4; i++) {
            p = verticies[i + o * 4];
            for (u32 j = 0; j < 4; j++) {
                a = verticies[j + 4 - o * 4];
                b = verticies[((j + 1) % 4) + 4 - o * 4];

                dist = point_segment_min_distance(p, a, b);

                if (fequal(dist, min_dist1)) {
                    if (!(fequal(p.x, p.y) && fequal(points[0].x, points[0].y))) {
                        points[1] = p;
                        count = 2;
                    }
                }
                else if (dist < min_dist1) {
                    min_dist1 = dist;
                    points[0] = p;
                    count = 1;
                }
            }
        }
    }

    return count;
}

void phys_update() {
    u32 length = array_list_length(&state->phys_boxes);

    float depth;
    Vec2f normal;
    Phys_Box *box1;
    Phys_Box *box2;

    for (u32 it = 0; it < PHYS_ITERATIONS; it++) {

        for (u32 i = 0; i < length; i++) {
            box1 = state->phys_boxes[i];

            // @Important: This is a check if a pointer box1 is invalid pointer, it is done by specifically looking at x dimension, since that is part of data that is set to 0.0f when box1 is no longer used / nullified.
            if (fequal(box1->bound_box.dimensions.x, 0.0f)) { 
                array_list_unordered_remove(&state->phys_boxes, i);
                length = array_list_length(&state->phys_boxes);
                i--;
                continue;
            }

            if (!box1->dynamic) {
                continue;
            }

            box1->grounded = false;

            // Applying gravity.
            if (box1->gravitable) {
                box1->body.velocity = vec2f_sum(box1->body.velocity, vec2f_multi_constant(GRAVITY_ACCELERATION, state->t.delta_time * PHYS_ITERATION_STEP_TIME ));
            }

            // Applying velocities.
            box1->bound_box.center = vec2f_sum(box1->bound_box.center, vec2f_multi_constant(box1->body.velocity, state->t.delta_time * PHYS_ITERATION_STEP_TIME));
            box1->bound_box.rot += box1->body.angular_velocity * state->t.delta_time * PHYS_ITERATION_STEP_TIME;
            box1->body.mass_center = box1->bound_box.center;

            // Debug drawing.
            draw_line(box1->bound_box.center, vec2f_sum(box1->bound_box.center, vec2f_multi_constant(box1->body.velocity, state->t.delta_time * PHYS_ITERATION_STEP_TIME)), VEC4F_RED, &state->debug_vert_buffer);

            // Debug drawing.
            draw_quad_outline(obb_p0(&box1->bound_box), obb_p1(&box1->bound_box), VEC4F_RED, box1->bound_box.rot, &state->debug_vert_buffer);

        }

        Vec2f contacts[2];
        u32 contacts_count;
        // Collision.
        for (u32 i = 0; i < length; i++) {
            box1 = state->phys_boxes[i];
            for (u32 j = i + 1; j < length; j++) {
                box2 = state->phys_boxes[j];
                if (phys_sat_check_collision_obb(&box1->bound_box, &box2->bound_box)) { // @Speed: Need separate broad phase.
                    
                    // Fidning depth and normal of collision.
                    phys_sat_find_min_depth_normal(&box1->bound_box, &box2->bound_box, &depth, &normal);

                    // Calculating dot product to check if any objects are grounded.
                    float grounded_dot = vec2f_dot(vec2f_normalize(GRAVITY_ACCELERATION), normal);

                    if (box1->dynamic && !box2->dynamic) {
                        phys_resolve_static_obb_collision(&box1->bound_box, depth, vec2f_negate(normal));

                        if (grounded_dot > 0.7f)
                            box1->grounded = true;
                    }
                    else if (box2->dynamic && !box1->dynamic) {
                        phys_resolve_static_obb_collision(&box2->bound_box, depth, normal);

                        if (grounded_dot < -0.7f)
                            box2->grounded = true;
                    }
                    else {
                        phys_resolve_dynamic_obb_collision(&box1->bound_box, &box2->bound_box, depth, normal);

                        if (grounded_dot > 0.7f)
                            box1->grounded = true;
                        else if (grounded_dot < -0.7f)
                            box2->grounded = true;
                    }
                    box1->body.mass_center = box1->bound_box.center;
                    box2->body.mass_center = box2->bound_box.center;


                    /**
                     * @Todo: Remove physics resolution abstraction, and generalize for different types of phys boxes collisions.
                     */
                    contacts_count = phys_find_contanct_points_obb(&box1->bound_box, &box2->bound_box, contacts);
                    phys_resolve_phys_box_collision_with_rotation_friction(box1, box2, normal, contacts, contacts_count);
                }
            }
        }
    }
}












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
        
        printf("Delta Time Multiplier: %f\n", state->t.delta_time_multi);
    }

    // Player logic.
    update_player(&state->player);
    
    // Lerp camera.
    state->main_camera.center = vec2f_lerp(state->main_camera.center, state->player.p_box.bound_box.center, 0.025f);

    // Boxes logic.
    update_boxes();

    // Physics update loop.
    phys_update();




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
    UI_WINDOW(
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

    UI_WINDOW(
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

    UI_WINDOW(
        UI_FRAME(state->window.width, 50,
            if (UI_BUTTON(vec2f_make(100, 50), "Go Back")) {
                state->gs = PLAY;
            }
        );
    );

    state->ui.y_axis = UI_ALIGN_DEFAULT;

    UI_WINDOW(
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








void plug_init(Plug_State *s) {
    state = s;

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
    


    ui_init();
    phys_init();



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

void plug_load(Plug_State *s) {
    state = s;

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


    // Player loading.
    state->player.speed = 5.0f;

}


static s32 load_counter = 5;

/**
 * @Important: In game update loops the order of procedures is: Updating -> Drawing.
 * Where in updating all logic of the game loop is contained including inputs, sound and so on.
 * Drawing part is only responsible for putting pixels accrodingly with calculated data in "Updating" part.
 */
void plug_update(Plug_State *s) {
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
}

void plug_unload(Plug_State *s) {
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
















