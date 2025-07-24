#ifndef EVENT_H
#define EVENT_H

#include "game/graphics.h"

#include "core/core.h"
#include "core/mathf.h"

#include <stdbool.h>

typedef struct mouse_input {
    Vec2f position;
    bool left_hold;
    bool left_pressed;
    bool left_unpressed;
    bool right_hold;
    bool right_pressed;
    bool right_unpressed;
} Mouse_Input;

typedef struct text_input {
    bool text_inputted;
    char buffer[32];    // Size of SDL text input buffer.
} Text_Input;

typedef struct events_info {
    bool should_quit;
    Mouse_Input mouse_input;
    Text_Input text_input;
} Events_Info;

void init_events_handler(Events_Info *events);

void handle_events(Events_Info *events, Window_Info *window, Time_Info *t);


/**
 * This function will insert input text (if any) to the buffer specified.
 * Starting at write index it's going to shift everything until length to the right and insert the text.
 * If inserted length is greater than buffer's capacity it will stop and return 0.
 * It will return the amount of bytes inserted.
 */
s64 insert_input_text(char *buffer, s64 capacity, s64 length, s64 write_index, Text_Input *text_input);

#endif
