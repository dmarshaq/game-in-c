#include "game/console.h"
#include "draw.h"
#include "game/plug.h"
#include "game/draw.h"
#include "core/graphics.h"
#include "core/structs.h"
#include "core/str.h"
#include "core/mathf.h"
#include "core/file.h"
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <stdio.h>


static Quad_Drawer *drawer;

static Font_Baked font_input;
static Font_Baked font_output;

static Matrix4f projection;

static const float SPEED = 100;
static const float OPEN_PERCENT = 0.4f;
static const float FULL_OPEN_PERCENT = 0.8f;
static const float TEXT_PAD = 10;
static const s64 HISTORY_BUFFER_SIZE = 1024;
static const s64 HISTORY_MAX_STRINGS = 64;


static String history[64];
static s64 history_length;
static float history_font_top_pad;
static float history_block_width;

static char input[100] = "";
static float input_height;
static float input_font_top_pad;
static float input_block_width;

static s64 input_cursor_index;
static T_Interpolator input_cursor_blink_timer;
static bool input_cursor_visible;
static float input_cursor_activity;

static float c_y0;
static float c_y0_target;

static float c_x0;
static float c_x1;


static Arena_Allocator history_arena;



typedef enum console_openness {
    CLOSED,
    OPEN,
    FULLY_OPEN,
} Console_Openness;

static Console_Openness console_state;


float console_max_height(Window_Info *window){
    return window->height * FULL_OPEN_PERCENT; 
}

void console_start_input_if_not(Text_Input *text_i) {
    if (!SDL_IsTextInputActive()) {

        SDL_StartTextInput();

        text_i->buffer = input;
        text_i->capacity = 99; // @Important: The last symbol is reserved for the new line in the input.
        text_i->length = strlen(input);
        text_i->write_index = text_i->length;
    }
}

void console_stop_input(Text_Input *text_i) {
    SDL_StopTextInput();

    text_i->buffer = NULL;
    text_i->capacity = 0;
    text_i->length = 0;
    text_i->write_index = 0;
}


void init_console(Plug_State *state) {
    // Get resources.
    drawer = &state->quad_drawer;

    // Load needed font.
    u8* font_data = read_file_into_buffer("res/font/Consolas-Regular.ttf", NULL, &std_allocator);
    font_input = font_bake(font_data, 18.0f);
    font_output = font_bake(font_data, 16.0f);
    free(font_data);

    // @Important: For metrics we assume that fonts are monospaced!
    // Set input metrics.
    input_font_top_pad = font_input.line_height * 0.4f;
    input_height = font_input.line_height + input_font_top_pad;
    input_block_width = font_input.chars[(s32)' ' - font_input.first_char_code].xadvance;

    // Set history height.
    history_font_top_pad = font_output.line_height * 0.2f;
    history_block_width = font_output.chars[(s32)' ' - font_input.first_char_code].xadvance;

    // Cursor.
    input_cursor_blink_timer = ti_make(800);
    input_cursor_visible = true;
    input_cursor_activity = 0.0f;

    // Console positions and state.
    c_y0 = state->window.height;
    console_state = OPEN;
    console_start_input_if_not(&state->events.text_input);


    // Initing allocator for history. @Temporary: Later should done using looped array.
    history_arena = arena_make(HISTORY_BUFFER_SIZE);
    history_length = 0;


    // Test of multiline output.
    String test = STR_BUFFER("This is just a test of a ...\nMultiline console output!\n    It should work fine\n");
    console_add(test.data, test.length);

    cprintf("Hello world!\n");
    cprintf("Wait is this actually cool strings? %4d\n", 103);
}

void console_update(Window_Info *window, Events_Info *events, Time_Info *t) {
    c_x0 = 0.1f * window->width;
    c_x1 = 0.9f * window->width;

    // Checking for input to change console's state.
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

    // Lerping console if it's y0 doesn't match it's state (target y0).
    switch(console_state) {
        case CLOSED:
            c_y0_target = window->height;
            break;
        case OPEN:
            c_y0_target = window->height * (1.0f - OPEN_PERCENT);
            break;
        case FULLY_OPEN:
            c_y0_target = window->height * (1.0f - FULL_OPEN_PERCENT);
            break;
        default:
            printf_err("Unknown console state.\n");
            break;
    }

    c_y0 = lerp(c_y0, c_y0_target, 0.01f * t->delta_time_milliseconds);

    // Updating cursor index. @Refactor: Make it depends on console states.
    if (SDL_IsTextInputActive()) {
        if (pressed(SDLK_RETURN)) {
            // Add a new line symbol and flush input if return key was pressed.
            events->text_input.buffer[events->text_input.length] = '\n';
            console_add(events->text_input.buffer, events->text_input.length + 1);

            // Clearing text input buffer.
            events->text_input.buffer[0] = '\0';
            events->text_input.length = 0;
            events->text_input.write_index = 0;
        }

        input_cursor_index = events->text_input.write_index;
    }

    // Cursor styling.
    ti_update(&input_cursor_blink_timer, t->delta_time_milliseconds);
    
    if (events->text_input.write_moved) {
        ti_reset(&input_cursor_blink_timer);
        input_cursor_visible = true;
        input_cursor_activity += 0.03f;
    } else {
        input_cursor_activity -= 0.15f * t->delta_time;
    }
    input_cursor_activity = clamp(input_cursor_activity, 0.0f, 0.25f);


    if (ti_is_complete(&input_cursor_blink_timer)) {
        ti_reset(&input_cursor_blink_timer);
        input_cursor_visible = !input_cursor_visible;
    }

}

void console_draw(Window_Info *window) {
    projection = screen_calculate_projection(window->width, window->height);
    shader_update_projection(drawer->program, &projection);
    

    draw_begin(drawer);

    // Output.
    draw_quad(vec2f_make(c_x0, c_y0 + input_height), vec2f_make(c_x1, c_y0 + console_max_height(window)), vec4f_make(0.10f, 0.12f, 0.24f, 0.98f), NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, 0, NULL);

    // Input.
    draw_quad(vec2f_make(c_x0, c_y0), vec2f_make(c_x1, c_y0 + input_height), vec4f_make(0.18f, 0.18f, 0.35f, 0.98f), NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, 0, NULL);
    
    // Draw text of the history.
    Vec2f history_draw_origin = vec2f_make(c_x0 + TEXT_PAD, c_y0 + input_height);
    String str;
    for (s64 i = 1; i <= history_length; i++) {
        str = history[history_length - i];
        history_draw_origin.y += text_size_y(str, &font_output); 
        draw_text(str, history_draw_origin, VEC4F_WHITE, &font_output, 1, NULL);
        // history_draw_origin.y += history_font_top_pad;
    }
    
    
    // Draw input cursor.
    if (input_cursor_visible) {
        Vec4f color = vec4f_lerp(vec4f_make(0.58f, 0.58f, 0.85f, 0.90f), VEC4F_YELLOW, input_cursor_activity);
        draw_quad(vec2f_make(c_x0 + TEXT_PAD + input_cursor_index * input_block_width, c_y0 + font_input.line_gap), vec2f_make(c_x0 + TEXT_PAD + (input_cursor_index + 1) * input_block_width, c_y0 + input_height - input_font_top_pad * 0.5f), color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, 0, NULL);
    }


    // Draw input text.
    draw_text(CSTR(input), vec2f_make(c_x0 + TEXT_PAD, c_y0 + input_height - input_font_top_pad), VEC4F_CYAN, &font_input, 1, NULL);

    draw_end();
}



void console_free() {
    arena_destroy(&history_arena);
}



void console_add(char *buffer, s64 length) {
    if (history_length < HISTORY_MAX_STRINGS) {
        char *ptr = allocator_alloc(&history_arena, length);
        if (ptr == NULL) {
            printf_err("Console's buffer is out of memory, cannot add more strings.\n");
            return;
        }

        memcpy(ptr, buffer, length);

        // Following code will determine to either create a new String if previous was ended with '\n' or append to previous string if not. It is done to properly display messages if previous one wasn't ended with '\n'.
        if (history_length > 0 && history[history_length - 1].data[history[history_length - 1].length - 1] != '\n') {
            history[history_length - 1].length += length;
        } else {
            history[history_length] = STR(length, ptr);
            history_length++;
        }

    } else {
        printf_err("Max console string limit has been reached, cannot add more strings.\n");
    }
}

void cprintf(char *format, ...) {
     va_list args;
     va_start(args, format);

     // Make a copy, because vsnprintf consumes args, so this will make consume only the copy.
     va_list args_copy;
     va_copy(args_copy, args);
     s64 length = vsnprintf(NULL, 0, format, args_copy);
     va_end(args_copy);

     if (length < 0) {
         printf_err("Failed to find formatted string length for console output.\n");
         return;
     }

     // String formatted_str = STR(length + 1, (char[length]));
     char buffer[length + 1];
     vsnprintf(buffer, length + 1, format, args);

     va_end(args);

     console_add(buffer, length);
}


