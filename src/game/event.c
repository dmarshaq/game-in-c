#include "game/event.h"
#include "core/type.h"
#include "core/input.h"
#include "core/mathf.h"

#include "game/plug.h"
#include "plug.h"

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_mouse.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


static T_Interpolator key_timer = ti_make(500);
static T_Interpolator key_repeat_timer = ti_make(50);
static SDL_Event event;

void init_events_handler(Events_Info *events) {
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
        .buffer = NULL,
        .capacity = 0,
        .length = 0,
        .write_index = 0,
    };
    
    // Make sure text input is disabled on start / init.
    if (SDL_IsTextInputActive()) {
        SDL_StopTextInput();
    }
}

void handle_text_modification(Events_Info *events, Time_Data *t) {
    Text_Input *info = &events->text_input;

    if (info->buffer == NULL || info->capacity <= 0) {
        printf_err("Cannot handle text modification: output buffer is either NULL or its capacity is less then or equal to 0\n");
        return;
    }

    // Character deletion.
    if (info->write_index > 0) {

        if (pressed(SDLK_BACKSPACE)) {

            // Shifting string if there is insertion.
            if (info->write_index < info->length) {

                // Shift the string to the left by one.
                for (s64 i = info->write_index - 1; i < info->length; i++) {
                    info->buffer[i] = info->buffer[i + 1];
                }
            }


            info->write_index--;
            info->length--;

            // To satisfy C NULL termination string.
            info->buffer[info->length] = '\0';


            ti_reset(&key_timer);


            return;
        }

        if (hold(SDLK_BACKSPACE)) {
            ti_update(&key_timer, t->delta_time_milliseconds);

            if (ti_is_complete(&key_timer)) {

                ti_update(&key_repeat_timer, t->delta_time_milliseconds);

                if (ti_is_complete(&key_repeat_timer)) {


                    // Shifting string if there is insertion.
                    if (info->write_index < info->length) {

                        // Shift the string to the left by one.
                        for (s64 i = info->write_index - 1; i < info->length; i++) {
                            info->buffer[i] = info->buffer[i + 1];
                        }
                    }

                    info->write_index--;
                    info->length--;

                    // To satisfy C NULL termination string.
                    info->buffer[info->length] = '\0';


                    ti_reset(&key_repeat_timer);
                }
            }

            return;
        }
    }

    // Arrow key left.
    if (pressed(SDLK_LEFT)) {

        info->write_index--;

        ti_reset(&key_timer);

        goto leave_clamp;
    }

    if (hold(SDLK_LEFT)) {
        ti_update(&key_timer, t->delta_time_milliseconds);

        if (ti_is_complete(&key_timer)) {

            ti_update(&key_repeat_timer, t->delta_time_milliseconds);

            if (ti_is_complete(&key_repeat_timer)) {
                
                info->write_index--;

                ti_reset(&key_repeat_timer);
            }
        }

        goto leave_clamp;
    }


    // Arrow key right.
    if (pressed(SDLK_RIGHT)) {

        info->write_index++;

        ti_reset(&key_timer);

        goto leave_clamp;
    }

    if (hold(SDLK_RIGHT)) {
        ti_update(&key_timer, t->delta_time_milliseconds);

        if (ti_is_complete(&key_timer)) {

            ti_update(&key_repeat_timer, t->delta_time_milliseconds);

            if (ti_is_complete(&key_repeat_timer)) {
                
                info->write_index++;

                ti_reset(&key_repeat_timer);
            }
        }

        goto leave_clamp;
    }


leave_clamp:
    // Make sure write index is clamped before returning, because arrow keys don't explicitly check bounds when moving write index.
    info->write_index = clampi(info->write_index, 0, info->length);
    return;
}

void handle_text_input(Events_Info *events) { 
    Text_Input *info = &events->text_input;

    if (info->buffer == NULL || info->capacity <= 0) {
        printf_err("Cannot handle text input: output buffer is either NULL or its capacity is less then or equal to 0\n");
        return;
    }

    s64 input_length = strlen(event.text.text);


    if (input_length > 0) {
        // Check for buffer overflow.
        // @Important: -1 is always there, because the last character in the buffer is always NULL termination, so that in case of buffer being fully filled it is still considered safe C NULL terminated string.
        if (info->length + input_length > info->capacity - 1) {
            return;
        }


        // Shifting string if there is insertion.
        if (info->write_index < info->length) {

            // Shift the string to the right on the amount of inputted characters.
            for (s64 i = info->length + input_length - 1; i > info->write_index + input_length - 1; i--) {
                info->buffer[i] = info->buffer[i - input_length];
            }
        }

        // Using memcpy cause strcpy also copies null terminator, but since null terminator is managed manually it is not needed to happen.
        memcpy(info->buffer + info->write_index, event.text.text, input_length);


        // Advancing.
        info->write_index += input_length;
        info->length += input_length;

        
        // To satisfy C NULL termination string.
        info->buffer[info->length] = '\0';
    }
}


void handle_events(Events_Info *events, Window_Info *window, Time_Data *t) {
    // Clear inputs.
    events->mouse_input.left_pressed = false;
    events->mouse_input.left_unpressed = false;
    events->mouse_input.right_pressed = false;
    events->mouse_input.right_unpressed = false;

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
                handle_text_input(events);
                break;
            default:
                break;
        }
    }

    if (SDL_IsTextInputActive()) {
        handle_text_modification(events, t);
    }
}






