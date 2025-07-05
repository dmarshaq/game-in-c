#include "game/console.h"
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




// Commands.
typedef enum type {
    INTEGER,
    FLOAT,
    STRING,
} Type;


#define arg_int(value)      ((Command_Argument){ INTEGER, .int_value = value })
#define arg_float(value)    ((Command_Argument){ FLOAT,   .float_value = value })
#define arg_str(value)      ((Command_Argument){ STRING,  .str_value = CSTR(value) })

typedef struct command_argument {
    Type type;
    union {
        s64 int_value;
        float float_value;
        String str_value;
    };
} Command_Argument;

typedef struct command {
    String name;
    void (*func)(Command_Argument *, u32);
    u32 min_args;
    u32 max_args;
    Command_Argument *args;
} Command;

static Command *commands;

static Arena_Allocator commands_arena;









float console_max_height(Window_Info *window){
    return window->height * FULL_OPEN_PERCENT; 
}

void console_start_input_if_not(Text_Input *text_i) {
    if (!SDL_IsTextInputActive()) {

        SDL_StartTextInput();

        text_i->buffer = input;
        text_i->capacity = 100;
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
    // Init commands.
    init_console_commands();



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
            console_command(STR(events->text_input.length, events->text_input.buffer));

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




// Example function that we want to register (doesn't have to be in this file).
s64 add(s64 a, s64 b) {
    return a + b;
}


#define COMMAND_PREFIX(name)    _command_##name

void print_argument(Command_Argument *argument) {
    switch (argument->type) {
        case INTEGER:
            cprintf("Type: Integer    Value: %4d\n", argument->int_value);
            break;
        case FLOAT:
            cprintf("Type: Float      Value: %4.2\n", argument->float_value);
            break;
        case STRING:
            cprintf("Type: String     Value: %-10.*s\n", UNPACK(argument->str_value));
            break;
        default:
            cprintf("Type: Unknown\n");
            break;
    }
}

void print_command(Command *command) {
    cprintf("Name: %-10.*s    Minimum args: %2d    Maximum args: %2d\nArguments\n", UNPACK(command->name), command->min_args, command->max_args);

    for (u32 i = 0; i < command->max_args; i++) {
        print_argument(command->args + i);
    }
}



//////// CONSOLE WRAPPER COMMANDS GO HERE
void COMMAND_PREFIX(add)(Command_Argument *args, u32 args_length) {
    s64 result = add(args[0].int_value, args[1].int_value);
    cprintf("%d + %d = %d\n", args[0].int_value, args[1].int_value, result);
}

void COMMAND_PREFIX(spawn_box)(Command_Argument *args, u32 args_length) {
    spawn_box(vec2f_make(args[0].float_value, args[1].float_value), VEC4F_GREEN);
}





/**
 * 'min_args' <= 'max_args' must be true.
 */
void register_command(char *name, void (*ptr)(Command_Argument *, u32), u32 min_args, u32 max_args, Command_Argument *args) {
    Command_Argument *args_cpy = allocator_alloc(&commands_arena, max_args * sizeof(Command_Argument));
    memcpy(args_cpy, args, max_args * sizeof(Command_Argument));

    array_list_append(&commands, ((Command){ CSTR(name), ptr, min_args, max_args, args_cpy }));
}


void init_console_commands() {
    commands = array_list_make(Command, 8, &std_allocator); // @Leak.
    commands_arena = arena_make(256); // @Leak.
                    
    // Register commands here.
    register_command("add", COMMAND_PREFIX(add), 1, 2, (Command_Argument[2]) { arg_int(0), arg_int(0) } );
    register_command("spawn_box", COMMAND_PREFIX(spawn_box), 0, 2, (Command_Argument[2]) { arg_float(0), arg_float(0) } );
}





void console_command(String command) {
    cprintf("%.*s\n", UNPACK(command));

    String str = command;

    str = str_eat_spaces(str);
    if (str.length == 0) {
        cprintf("ERROR: Command name is not specified.\n");
        return;
    }
    
    String command_name = str_get_until_space(str);
    str.data += command_name.length;
    str.length -= command_name.length;

    for (s64 i = 0; i < array_list_length(&commands); i++) {
        if (str_equals(command_name, commands[i].name)) {
            

            // Scanning for arguments.
            Command_Argument parsed_args[commands[i].max_args];
            u32 args_count = 0;
            String arg;

            while (true) {
                str = str_eat_spaces(str);
                if (str.length == 0) {
                    break;
                }

                args_count++;
                if (args_count > commands[i].max_args) {
                    if (commands[i].max_args == 1)
                        cprintf("Expected no more than '%d' argument for '%.*s' command.\n", commands[i].max_args, UNPACK(commands[i].name));
                    else
                        cprintf("Expected no more than '%d' arguments for '%.*s' command.\n", commands[i].max_args, UNPACK(commands[i].name));
                    return;
                }

                // Getting arg string and moving str to the index after.
                arg = str_get_until_space(str);
                str.data += arg.length;
                str.length -= arg.length;

                // Parsing arg string into actual Command_Argument.
                // cprintf("Argument [%d]: '%.*s'.\n", args_count - 1, UNPACK(arg));

                switch(commands[i].args[args_count - 1].type) {
                    case INTEGER:
                        if (!str_is_int(arg)) {
                            cprintf("Argument [%d]: '%.*s' type mismatch, expected 'Integer'.\n", args_count - 1, UNPACK(arg));
                            return;
                        }
                        parsed_args[args_count - 1] = arg_int(str_parse_int(arg));

                        break;
                    case FLOAT:
                        if (!str_is_float(arg)) {
                            cprintf("Argument [%d]: '%.*s' type mismatch, expected 'Float'.\n", args_count - 1, UNPACK(arg));
                            return;
                        }
                        parsed_args[args_count - 1] = arg_float(str_parse_float(arg));

                        break;
                    case STRING:
                        parsed_args[args_count - 1] = (Command_Argument){ STRING, .str_value = arg };

                        break;
                }

            }

            if (args_count < commands[i].min_args) {
                if (commands[i].min_args == 1)
                    cprintf("Expected at least '%d' argument for '%.*s' command.\n", commands[i].min_args, UNPACK(commands[i].name));
                else
                    cprintf("Expected at least '%d' arguments for '%.*s' command.\n", commands[i].min_args, UNPACK(commands[i].name));
                return;
            }

            // Adding default args if not all args were filled.
            for (u32 j = args_count; j < commands[i].max_args; j++) {
                parsed_args[j] = commands[i].args[j];
            }



            // cprintf("Found command: '%.*s'.\n", UNPACK(commands[i].name));
            // print_command(commands + i);

            // This is default case for every command if no other args were specified.
            // Maybe...
            commands[i].func(parsed_args, commands[i].max_args);
            return;
        }
    }

    cprintf("ERROR: Unknown command: '%.*s'.\n", UNPACK(command_name));
}







