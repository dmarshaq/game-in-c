#ifndef INPUT_H
#define INPUT_H

#include "core/mathf.h"

#include <SDL2/SDL_keycode.h>
#include <stdbool.h>

/**
 * Inits keyboard state.
 * Internally allocates buffer for number of the keys to hold keyboard state.
 */
void keyboard_state_init();

/**
 * Updates old state to the recent one.
 */
void keyboard_state_old_update();

/**
 * Frees keyboard state.
 */
void keyboard_state_free();


/**
 * Returns true if specified key is currently held.
 */
bool hold(SDL_KeyCode key);

/**
 * Returns true if specified key was just pressed.
 */
bool pressed(SDL_KeyCode key);

/**
 * Returns true if specified key was just unpressed.
 */
bool unpressed(SDL_KeyCode key);

#endif
