#include "game/console.h"
#include "game/plug.h"
#include "game/draw.h"
#include "core/graphics.h"
#include "core/structs.h"
#include "core/str.h"
#include "core/mathf.h"
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>


static Quad_Drawer *drawer;

static Font_Baked *font_input;
static Font_Baked *font_output;

static Matrix4f projection;

static const float HEIGHT = 600;
static const float INPUT_HEIGHT = 20;
static const float CONSOLE_SPEED = 1000;

static String history[32];
static s64 history_length;
static char input[100] = "Input text here!";
static s64 input_cursor_index;

static float c_y0;


typedef enum console_openness {
    CLOSED,
    OPEN,
    FULLY_OPEN,
} Console_Openness;

static Console_Openness console_state;


void init_console(Plug_State *state) {
    // Get resources.
    drawer = &state->quad_drawer;
    font_input = hash_table_get(&state->font_table, "medium");
    font_output = hash_table_get(&state->font_table, "small");



    c_y0 = state->window.height;
    console_state = FULLY_OPEN;

    // Testing.
    // history[0] = CSTR("Hello world!\n");
    // history[1] = CSTR("This is just a test!\n");
    // history[2] = CSTR("Of console history\n");

    history_length = 0;
}

void console_start_input_if_not(Text_Input *text_i) {
    if (!SDL_IsTextInputActive()) {

        SDL_StartTextInput();

        text_i->buffer = input;
        text_i->capacity = 100;
        text_i->length = strlen(input);
        text_i->write_index = text_i->length;
        printf("Unknown SWITCH.\n");
    }
}

void console_stop_input(Text_Input *text_i) {
    SDL_StopTextInput();

    text_i->buffer = NULL;
    text_i->capacity = 0;
    text_i->length = 0;
    text_i->write_index = 0;
}

void console_update(Window_Info *window, Events_Info *events, Time_Data *t) {

    if (pressed(SDLK_F11)) {
        if (hold(SDLK_LSHIFT)) {
            if (console_state == FULLY_OPEN) {
                console_state = CLOSED;
                console_stop_input(&events->text_input);
            }
            else {
                console_state = FULLY_OPEN;
                console_start_input_if_not(&events->text_input);
            }
        } else {
            if (console_state == OPEN) {
                console_state = CLOSED;
                console_stop_input(&events->text_input);
            }
            else {
                console_state = OPEN;
                console_start_input_if_not(&events->text_input);
            }
        }
    }

    printf("Unknown Error debug.\n");
    switch(console_state) {
        case CLOSED:
            if (c_y0 < window->height)
                c_y0 = c_y0 + CONSOLE_SPEED * t->delta_time;
            break;
        case OPEN:
            if (c_y0 > window->height - HEIGHT * 0.4f)
                c_y0 = c_y0 - CONSOLE_SPEED * t->delta_time;
            else if (c_y0 < window->height - HEIGHT * 0.4f)
                c_y0 = c_y0 + CONSOLE_SPEED * t->delta_time;
            break;
        case FULLY_OPEN:
            if (c_y0 > window->height - HEIGHT)
                c_y0 = c_y0 - CONSOLE_SPEED * t->delta_time;
            break;
        default:
            printf_err("Unknown console state.\n");
            break;
    }

    c_y0 = clamp(c_y0, window->height - HEIGHT, window->height);

    // Updating cursor index.
    if (SDL_IsTextInputActive()) {
        input_cursor_index = events->text_input.write_index;
    }
}

void console_draw(Window_Info *window) {
    projection = screen_calculate_projection(window->width, window->height);
    shader_update_projection(drawer->program, &projection);
    

    draw_begin(drawer);

    // Output.
    draw_quad(vec2f_make(0, c_y0 + INPUT_HEIGHT), vec2f_make(window->width, c_y0 + HEIGHT), vec4f_make(0.10f, 0.12f, 0.24f, 0.98f), NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, 0, NULL);

    // Input.
    draw_quad(vec2f_make(0, c_y0), vec2f_make(window->width, c_y0 + INPUT_HEIGHT), vec4f_make(0.18f, 0.18f, 0.35f, 0.98f), NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, 0, NULL);
    
    float pad = 10;
    float line_height = 20;
    float cursor_pos_x = text_size(input, input_cursor_index, font_input).x;
    Vec2f cursor = vec2f_make(10, 16);

    // Draw text of the history.
    for (s64 i = 1; i <= history_length; i++) {
        // draw_text(history[history_length - i].data, vec2f_make(pad, c_y0 + INPUT_HEIGHT + line_height * i), VEC4F_WHITE, font_output, 1, NULL);
    }

    // Draw input text.
    draw_text(input, vec2f_make(pad + 2, c_y0 + line_height - 2), VEC4F_BLACK, font_input, 1, NULL);
    draw_text(input, vec2f_make(pad, c_y0 + line_height), VEC4F_WHITE, font_input, 1, NULL);

    // Draw input cursor.
    draw_quad(vec2f_make(pad + cursor_pos_x, c_y0 + line_height - cursor.y), vec2f_make(pad + cursor_pos_x + cursor.x, c_y0 + line_height), vec4f_make(0.58f, 0.58f, 0.85f, 0.96f), NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, 0, NULL);

    draw_end();
}


