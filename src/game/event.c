#include "game/event.h"
#include "game/input.h"
#include "game/graphics.h"

#include "core/type.h"
#include "core/mathf.h"

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


static T_Interpolator key_timer = ti_make(450);
static T_Interpolator key_repeat_timer = ti_make(30);
static SDL_Event event;

void event_init_handler(Events_Info *events) {
    events->should_quit = false;

    events->mouse_input = (Mouse_Input) {
        .position = VEC2F_ORIGIN,
        .left_hold = false,
        .left_pressed = false,
        .left_unpressed = false,
        .right_hold = false,
        .right_pressed = false,
        .right_unpressed = false,
    };

    events->text_input = (Text_Input) {
        .text_inputted = false,
        .buffer[0] = '\0',
    };
    
    // Make sure text input is disabled on start / init.
    if (SDL_IsTextInputActive()) {
        SDL_StopTextInput();
    }
}

void event_handle(Events_Info *events, Window_Info *window, Time_Info *t) {

    // Clear inputs.
    events->mouse_input.left_pressed = false;
    events->mouse_input.left_unpressed = false;
    events->mouse_input.right_pressed = false;
    events->mouse_input.right_unpressed = false;

    events->text_input.text_inputted = false;
    events->text_input.buffer[0] = '\0';

    // Poll events.
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                events->should_quit = true;
                break;
            case SDL_MOUSEMOTION:
                s32 mx, my;
                SDL_GetMouseState(&mx, &my);

                events->mouse_input.position.x = (float)mx;
                events->mouse_input.position.y = (float)(window->height - my);
                break;
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    events->mouse_input.left_pressed = true;
                    events->mouse_input.left_hold = true;
                }
                else if (event.button.button == SDL_BUTTON_RIGHT) {
                    events->mouse_input.right_pressed = true;
                    events->mouse_input.right_hold = true;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    events->mouse_input.left_unpressed = true;
                    events->mouse_input.left_hold = false;
                }
                else if (event.button.button == SDL_BUTTON_RIGHT) {
                    events->mouse_input.right_unpressed = true;
                    events->mouse_input.right_hold = false;
                }
                break;
            case SDL_TEXTINPUT:
                memcpy(events->text_input.buffer, event.text.text, strlen(event.text.text));
                events->text_input.text_inputted = true;

                break;
            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    window->width = event.window.data1;
                    window->height = event.window.data2;
                } 
            default:
                break;
        }
    }

}





s64 insert_input_text(char *buffer, s64 capacity, s64 length, s64 write_index, Text_Input *input) {
    if (capacity <= 0) {
        printf_err("Cannot handle text input: output buffer capacity is less then or equal to 0\n");
        return 0;
    }

    s64 input_length = strlen(input->buffer);

    // Check for buffer overflow.
    if (length + input_length > capacity) {
        return 0;
    }


    // Shifting string if there is insertion.
    if (write_index < length) {

        // Shift the string to the right on the amount of inputted characters.
        for (s64 i = length + input_length - 1; i > write_index + input_length - 1; i--) {
            buffer[i] = buffer[i - input_length];
        }
    }

    // Using memcpy cause strcpy also copies null terminator, but since null terminator is managed manually it is not needed to happen.
    memcpy(buffer + write_index, input->buffer, input_length);

    return input_length;
}





