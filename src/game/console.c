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
static const s64 HISTORY_BUFFER_SIZE = 8192;

#define HISTORY_MAX_MESSAGES  256
#define HISTORY_MAX_BUFFERS   2
#define INPUT_BUFFER_SIZE     100

typedef enum history_message_type : u8 {
    MESSAGE_USER        = 0,
    MESSAGE_LOG         = 1,
    MESSAGE_ERROR       = 2,
} History_Message_Type;

static const Vec4f HISTORY_MESSAGE_COLORS[] = {
    ((Vec4f) { 0.5f, 0.8f, 0.3f, 1.0f }),
    ((Vec4f) { 0.8f, 0.8f, 0.8f, 1.0f }),
    ((Vec4f) { 0.8f, 0.4f, 0.4f, 1.0f }),
};

typedef struct history_message {
    History_Message_Type type;
    String str;
} History_Message;

static History_Message *history;
static s64 history_index;
static float history_font_top_pad;
static float history_block_width;

static s64 display_line_offset; // It is amount of line that should be skipped before rendering first (most bottom line) in the console.
                                // So in case of scrolling console up this value would correspond to the amount of lines scrolled up.




/**
 * This structs sole purpose is to point to the specific user input relatively to the user input history array.
 * String cannot be used since they hold absolute address of the data not the realtive one to the beginning of the array.
 */
typedef struct user_input_handle {
    s64 length;
    s64 index;
} User_Input_Handle;

static User_Input_Handle *user_input_history;
static s64 user_input_peeked_message_index;


static char input[INPUT_BUFFER_SIZE] = "";
static float input_height;
static float input_font_top_pad;
static float input_block_width;

static s64 input_length;
static s64 input_cursor_index;

static T_Interpolator input_cursor_blink_timer;
static bool input_cursor_visible;
static bool input_cursor_moved;
static float input_cursor_activity;

static float c_y0;
static float c_y0_target;

static float c_x0;
static float c_x1;



static char *history_active_buffer;
static s64 history_active_buffer_index;
static s64 history_buffer_write_index;
static char *history_buffers[HISTORY_MAX_BUFFERS];

static char *user_input_history_buffer;


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
    }
}

void console_stop_input(Text_Input *text_i) {
    SDL_StopTextInput();
}

// @Refactor: This code is bad.
void history_peek_up_user_message() {
    if (user_input_peeked_message_index == -1) {
        user_input_peeked_message_index = (s64)array_list_length(&user_input_history) - 1;
    }
    else {
        user_input_peeked_message_index--;
        if (user_input_peeked_message_index == -1)
            user_input_peeked_message_index = 0;
    }

    if (user_input_peeked_message_index != -1) {
        input_cursor_index = user_input_history[user_input_peeked_message_index].length - 1;
    }
}

// @Refactor: This code is bad.
void history_peek_down_user_message() {
    if (user_input_peeked_message_index == -1) {
        return;
    }

    u32 length = array_list_length(&user_input_history);
    user_input_peeked_message_index++;
    if (user_input_peeked_message_index == length) {
        user_input_peeked_message_index = -1;
        input_cursor_index = input_length;
        return;
    }

    input_cursor_index = user_input_history[user_input_peeked_message_index].length - 1;
}


void history_return_peeked_message() {
    if (user_input_peeked_message_index != -1) {
        User_Input_Handle handle = user_input_history[user_input_peeked_message_index];
        
        input_length = handle.length - 1; // handle.length - 1 because last char is '\n' so we skip it.
        memcpy(input, &user_input_history_buffer[handle.index], input_length);

        user_input_peeked_message_index = -1;
    }
}





/**
 * Console printing.
 */
void console_add(char *buffer, s64 length, History_Message_Type type) {
    // Copy actual string data.
    if (history_buffer_write_index + length > HISTORY_BUFFER_SIZE) {
        if (length > HISTORY_BUFFER_SIZE) {
            printf_err("Message written to the console buffer is too long, not enough memory space to store it.\n");
            return;
        }

        // Swapping buffers.
        // @Important: Right now it doesn't clear swapped buffer and doesn't track any pointers that migth have been left in the buffer, but it is assumed that buffers are simply big, so that by the time it tracks back to the used buffer all string pointers that pointed to the that memory would be gone.
        history_active_buffer_index = (history_active_buffer_index + 1) % HISTORY_MAX_BUFFERS;
        history_buffer_write_index = 0;
    }

    char *ptr = history_buffers[history_active_buffer_index] + history_buffer_write_index;
    history_buffer_write_index += length;
    memcpy(ptr, buffer, length);

    // Saving user input into user input history.
    if (type == MESSAGE_USER) {
        array_list_append(&user_input_history, ((User_Input_Handle){ .length = length, .index = array_list_length(&user_input_history_buffer) }));
        array_list_append_multiple(&user_input_history_buffer, buffer, length);
    }


    // Following code will determine to either create a new String if previous was ended with '\n' or append to previous string if not. It is done to properly display messages if previous one wasn't ended with '\n'.
    bool concat_with_last_message = false;
    History_Message *last_message = NULL;
    u32 history_length = looped_array_length(&history);
    if (history_length > 0) {
        last_message = history + _looped_array_map_index((void *)history, history_length - 1);
        concat_with_last_message = last_message->str.data[last_message->str.length - 1] != '\n';
    }


    if (concat_with_last_message) {
        last_message->str.length += length;
    } else {
        History_Message msg = (History_Message){
                    .type = type,
                    .str = STR(length, ptr),
                    };
        looped_array_append(&history, msg);
    }
}

void cprintf_va(History_Message_Type type, char *format, va_list args) {
    // Make a copy of args
    va_list args_copy;
    va_copy(args_copy, args);
    s64 length = vsnprintf(NULL, 0, format, args_copy);
    va_end(args_copy);

    if (length < 0) {
        printf_err("Failed to find formatted string length for console output.\n");
        return;
    }

    char buffer[length + 1];
    vsnprintf(buffer, length + 1, format, args);

    console_add(buffer, length, type);
}

void cprintf(History_Message_Type type, char *format, ...) {
    va_list args;
    va_start(args, format);
    cprintf_va(type, format, args);
    va_end(args);
}

void console_log(char *format, ...) {
    va_list args;
    va_start(args, format);
    cprintf_va(MESSAGE_LOG, format, args);
    va_end(args);
}


void console_error(char *format, ...) {
    va_list args;
    va_start(args, format);
    cprintf_va(MESSAGE_ERROR, format, args);
    va_end(args);
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

    // Important not styling, logic vars.
    history = looped_array_make(History_Message, HISTORY_MAX_MESSAGES, &std_allocator);
    display_line_offset = 0;


    user_input_history = array_list_make(User_Input_Handle, 8, &std_allocator);
    user_input_peeked_message_index = -1;



    input_length = 0; 
    input_cursor_index = 0;



    // Cursor.
    input_cursor_blink_timer = ti_make(800);
    input_cursor_visible = true;
    input_cursor_moved = false;
    input_cursor_activity = 0.0f;

    // Console positions and state.
    c_y0 = state->window.height;
    console_state = OPEN;
    console_start_input_if_not(&state->events.text_input);


    // Initing all history buffers. The idea is to swap buffer once it has been filled for the next one.
    history_active_buffer_index = 0;
    history_active_buffer = allocator_alloc(&std_allocator, HISTORY_BUFFER_SIZE * HISTORY_MAX_BUFFERS);
    for (s64 i = 0; i < HISTORY_MAX_BUFFERS; i++) {
        history_buffers[i] = history_active_buffer + i * HISTORY_BUFFER_SIZE;
    }
    history_buffer_write_index = 0;

    user_input_history_buffer = array_list_make(char, HISTORY_BUFFER_SIZE, &std_allocator);
}

void console_update(Window_Info *window, Events_Info *events, Time_Info *t) {
    // Reset state.
    input_cursor_moved = false;

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
    

    // Checking for input if active.
    if (SDL_IsTextInputActive()) {
        
        if (repeat(SDLK_UP, t->delta_time_milliseconds)) {
            if (hold(SDLK_LSHIFT))
                display_line_offset++;
            else
                history_peek_up_user_message();
        }

        if (repeat(SDLK_DOWN, t->delta_time_milliseconds)) {
            if (hold(SDLK_LSHIFT))
                display_line_offset--;
            else
                history_peek_down_user_message();
        }

        if (display_line_offset < 0) 
            display_line_offset = 0;

        if (repeat(SDLK_LEFT, t->delta_time_milliseconds)) {
            // Stop history command walk when cursor moving.
            history_return_peeked_message();
            
            if (input_cursor_index > 0)
                input_cursor_index--;

            input_cursor_moved = true;
        }

        if (repeat(SDLK_RIGHT, t->delta_time_milliseconds)) {
            // Stop history command walk when cursor moving.
            history_return_peeked_message();
            
            if (input_cursor_index < input_length)
                input_cursor_index++;

            input_cursor_moved = true;
        }


        if (events->text_input.text_inputted) {
            // Stop history command walk when text editing.
            history_return_peeked_message();

            s64 written = insert_input_text(input, INPUT_BUFFER_SIZE, input_length, input_cursor_index,&events->text_input);

            input_length += written;
            input_cursor_index += written;

            input_cursor_moved = true;
        }



        if (repeat(SDLK_BACKSPACE, t->delta_time_milliseconds)) {
            // Stop history command walk when text editing.
            history_return_peeked_message();

            if (input_cursor_index > 0) {
                // Shifting string if deleted in the middle of the text.
                if (input_cursor_index < input_length) {

                    // Shift the string to the left by one.
                    for (s64 i = input_cursor_index - 1; i < input_length; i++) {
                        input[i] = input[i + 1];
                    }
                }

                input_length--;
                input_cursor_index--;
            }

            input_cursor_moved = true;
        }




        if (pressed(SDLK_RETURN)) {
            // Stop history command walk when text editing.
            history_return_peeked_message();

            // Add a new line symbol and flush input if return key was pressed.
            cprintf(MESSAGE_USER, "%.*s\n", input_length, input);
            console_exec_command(STR(input_length, input));

            // Clearing text input buffer.
            input[0] = '\0';
            input_length = 0;
            input_cursor_index = 0;

            // Moving display of the console to the very bottom.
            display_line_offset = 0;
        }

        //  input_cursor_index = events->text_input.write_index;
    }

    // Cursor styling.
    ti_update(&input_cursor_blink_timer, t->delta_time_milliseconds);
    
    if (input_cursor_moved) {
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
    s64 lines_drawen = 0;
    History_Message *msg;
    s64 msg_line_count;

    u32 history_length = looped_array_length(&history);
    for (s64 i = 1; i <= history_length; i++) {
        msg = history + _looped_array_map_index((void *)history, history_length - i); // Calling to internal function cause it is just faster to get pointer this way.
        msg_line_count = str_count_chars(msg->str, '\n'); // Later when wrapping of text will be made in the console, the counting of lines will change.

        lines_drawen += msg_line_count;
        if (lines_drawen <= display_line_offset) {
            continue;
        }

        // Cutting off lines that are in the same message but are supposed to be offseted.
        String msg_str = msg->str;
        s64 msg_cut_lines = msg_line_count - mini(msg_line_count, lines_drawen - display_line_offset);
        for (s64 j = -1; j < msg_cut_lines; j++) { // @Important: -1 is here because this for loop is given extra turn to remove the very last '\n' at the end of every message.
            msg_str = str_substring(msg_str, 0, str_find_char_right(msg_str, '\n'));
        }


        history_draw_origin.y += (msg_line_count - msg_cut_lines) * font_output.line_height; 
        // history_draw_origin.y += history_font_top_pad;

        draw_text(msg_str, history_draw_origin, HISTORY_MESSAGE_COLORS[msg->type], &font_output, 1, NULL);
    }
    
    display_line_offset = mini(display_line_offset, lines_drawen);
    
    // Draw input cursor.
    if (input_cursor_visible) {
        Vec4f color = vec4f_lerp(vec4f_make(0.58f, 0.58f, 0.85f, 0.90f), VEC4F_YELLOW, input_cursor_activity);
        
        // Choosing cursor width.
        float width = input_block_width;
        if (input_cursor_index < input_length) {
            width = 2;
        }

        draw_quad(vec2f_make(c_x0 + TEXT_PAD + input_cursor_index * input_block_width, c_y0 + font_input.line_gap), vec2f_make(c_x0 + TEXT_PAD + input_cursor_index * input_block_width + width, c_y0 + input_height - input_font_top_pad * 0.5f), color, NULL, VEC2F_ORIGIN, VEC2F_UNIT, NULL, 0, NULL);
    }


    // Draw input text.
    if (user_input_peeked_message_index != -1) {
        User_Input_Handle handle = user_input_history[user_input_peeked_message_index];
        draw_text(STR(handle.length, &user_input_history_buffer[handle.index]), vec2f_make(c_x0 + TEXT_PAD, c_y0 + input_height - input_font_top_pad), VEC4F_YELLOW, &font_input, 1, NULL);
    } else {
        draw_text(STR(input_length, input), vec2f_make(c_x0 + TEXT_PAD, c_y0 + input_height - input_font_top_pad), VEC4F_CYAN, &font_input, 1, NULL);
    }

    draw_end();
}



void console_free() {
    allocator_free(&std_allocator, history_buffers[0]);
}










void print_argument(Command_Argument *argument) {
    switch (argument->type) {
        case INTEGER:
            console_log("Type: Integer       Default value: %-4d\n", argument->int_value);
            break;
        case FLOAT:
            console_log("Type: Float         Default value: %4.2f\n", argument->float_value);
            break;
        case STRING:
            console_log("Type: String        Default value: %-10.*s\n", UNPACK(argument->str_value));
            break;
        default:
            console_log("Type: Unknown\n");
            break;
    }
}

void print_command(Command *command) {
    console_log("\n\nName: %-10.*s    Minimum args: %2d    Maximum args: %2d\n", UNPACK(command->name), command->min_args, command->max_args);
    
    console_log("----------------------------------------------------------------\n");
    for (u32 i = 0; i < command->max_args; i++) {
        console_log("    Argument [%d]:   ", i);
        print_argument(command->args + i);
    }
}







/**
 * 'min_args' <= 'max_args' must be true.
 */
void register_command(char *name, void (*ptr)(Command_Argument *, u32), u32 min_args, u32 max_args, Command_Argument *args) {
    Command_Argument *args_cpy = allocator_alloc(&commands_arena, max_args * sizeof(Command_Argument));
    memcpy(args_cpy, args, max_args * sizeof(Command_Argument));

    array_list_append(&commands, ((Command){ CSTR(name), ptr, min_args, max_args, args_cpy }));
}






// Example function that we want to register (doesn't have to be in this file).
s64 add(s64 a, s64 b) {
    return a + b;
}

//////// CONSOLE WRAPPER COMMANDS GO HERE

#define COMMAND_PREFIX(name)    _command_##name

void COMMAND_PREFIX(help)(Command_Argument *args, u32 args_length) {
    if (args[0].str_value.length > 0) {
        console_log("Finding '%.*s' command . . .", UNPACK(args[0].str_value));
        for (u32 i = 0; i < array_list_length(&commands); i++) {
            if (str_equals(args[0].str_value, commands[i].name)) {
                console_log(" ok\n");
                print_command(commands + i);
                return;
            }
        }

        console_log(" x\n'%.*s' is not a command.\n", UNPACK(args[0].str_value));
        return;
    }

    console_log("Listing all available commands:\n");
    for (u32 i = 0; i < array_list_length(&commands); i++) {
        print_command(commands + i);
    }
}

void COMMAND_PREFIX(clear)(Command_Argument *args, u32 args_length) {
    looped_array_clear(&history);
}

void COMMAND_PREFIX(quit)(Command_Argument *args, u32 args_length) {
    quit();
}

void COMMAND_PREFIX(add)(Command_Argument *args, u32 args_length) {
    s64 result = add(args[0].int_value, args[1].int_value);
    console_log("%d + %d = %d\n", args[0].int_value, args[1].int_value, result);
}

void COMMAND_PREFIX(for)(Command_Argument *args, u32 args_length) {
    for (s64 i = args[0].int_value; i < args[1].int_value; i++) {
        console_log("%d\n", i);
    }
}

void COMMAND_PREFIX(spawn_box)(Command_Argument *args, u32 args_length) {
    spawn_box(vec2f_make(args[0].float_value, args[1].float_value), VEC4F_GREEN);
}

/////// MAKE COMMANDS ABOVE.

void init_console_commands() {
    commands = array_list_make(Command, 8, &std_allocator); // @Leak.
    commands_arena = arena_make(256); // @Leak.
                    
    // Register commands here.
    register_command("help",        COMMAND_PREFIX(help),       0, 1, (Command_Argument[1]) { arg_str("") });
    register_command("clear",       COMMAND_PREFIX(clear),      0, 0, (Command_Argument[0]) {  });
    register_command("quit",        COMMAND_PREFIX(quit),       0, 0, (Command_Argument[0]) {  });
    register_command("add",         COMMAND_PREFIX(add),        1, 2, (Command_Argument[2]) { arg_int(0), arg_int(0) } );
    register_command("for",         COMMAND_PREFIX(for),        2, 2, (Command_Argument[2]) { arg_int(0), arg_int(1) } );
    register_command("spawn_box",   COMMAND_PREFIX(spawn_box),  0, 2, (Command_Argument[2]) { arg_float(0), arg_float(0) } );
}




/**
 * @Speed: When list of commands grows it will be benificial to store them in a hash table.
 */
void console_exec_command(String command) {
    String str = command;

    str = str_eat_spaces(str);
    if (str.length == 0) {
        console_log("Command name is not specified.\n");
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
                        console_log("%.*s: Expected no more than '%d' argument for '%.*s' command.\n", UNPACK(command_name), commands[i].max_args, UNPACK(commands[i].name));
                    else
                        console_log("%.*s: Expected no more than '%d' arguments for '%.*s' command.\n", UNPACK(command_name), commands[i].max_args, UNPACK(commands[i].name));
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
                            console_log("%.*s: Argument [%d]: '%.*s' type mismatch, expected 'Integer'.\n", UNPACK(command_name), args_count - 1, UNPACK(arg));
                            return;
                        }
                        parsed_args[args_count - 1] = arg_int(str_parse_int(arg));

                        break;
                    case FLOAT:
                        if (!str_is_float(arg)) {
                            console_log("%.*s: Argument [%d]: '%.*s' type mismatch, expected 'Float'.\n", UNPACK(command_name), args_count - 1, UNPACK(arg));
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
                    console_log("%.*s: Expected at least '%d' argument for '%.*s' command.\n", UNPACK(command_name), commands[i].min_args, UNPACK(commands[i].name));
                else
                    console_log("%.*s: Expected at least '%d' arguments for '%.*s' command.\n", UNPACK(command_name), commands[i].min_args, UNPACK(commands[i].name));
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

    console_log("%.*s: Is not a command.\n", UNPACK(command_name));
}







