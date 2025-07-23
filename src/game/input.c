#include "game/input_meta_.h"

#include "core/core.h"
#include "core/type.h"
#include "core/mathf.h"

#include <SDL2/SDL_keycode.h>
#include <SDL2/SDL_events.h>
#include <stdbool.h>

/**
 * Input.
 */

static T_Interpolator key_timer = ti_make(450);
static T_Interpolator key_repeat_timer = ti_make(30);

static u8 *keyboard_state_old;
static const u8 *keyboard_state_current;
static u32 keyboard_number_of_keys;

void keyboard_state_free() {
    free(keyboard_state_old);
}

void keyboard_state_old_update() {
    (void)memcpy(keyboard_state_old, keyboard_state_current, keyboard_number_of_keys);
}

void keyboard_state_init() {
    keyboard_state_current = SDL_GetKeyboardState(&keyboard_number_of_keys);
    keyboard_state_old = malloc(keyboard_number_of_keys);
    keyboard_state_old_update();
}

bool hold(SDL_KeyCode key) {
    return *(keyboard_state_current + SDL_GetScancodeFromKey(key)) == 1;
}

bool pressed(SDL_KeyCode key) {
    return *(keyboard_state_old + SDL_GetScancodeFromKey(key)) == 0 && *(keyboard_state_current + SDL_GetScancodeFromKey(key)) == 1;
}

bool unpressed(SDL_KeyCode key) {
    return *(keyboard_state_old + SDL_GetScancodeFromKey(key)) == 1 && *(keyboard_state_current + SDL_GetScancodeFromKey(key)) == 0;
}

bool repeat(SDL_KeyCode key, float dt_milliseconds) {
    if (pressed(key)) {
        ti_reset(&key_timer);
        return true;
    }

    if (hold(key)) {
        ti_update(&key_timer, dt_milliseconds);

        if (ti_is_complete(&key_timer)) {

            ti_update(&key_repeat_timer, dt_milliseconds);

            if (ti_is_complete(&key_repeat_timer)) {

                ti_reset(&key_repeat_timer);
                return true;
            }
        }

    }

    return false;
}


