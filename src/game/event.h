#ifndef EVENT_H
#define EVENT_H

#include "core/core.h"
#include "core/graphics.h"
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
    char *buffer;
    s64 capacity;
    s64 length;
    s64 write_index;
    bool write_moved;
} Text_Input;

typedef struct events_info {
    bool should_quit;
    Mouse_Input mouse_input;
    Text_Input text_input;
} Events_Info;

void init_events_handler(Events_Info *events);

void handle_events(Events_Info *events, Window_Info *window, Time_Info *t);

#endif
