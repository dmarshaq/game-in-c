#ifndef CONSOLE_H
#define CONSOLE_H

#include "game/game.h"
#include "game/draw.h"
#include "game/graphics.h"

#include "core/structs.h"
#include "core/str.h"
#include "core/mathf.h"

#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>



void init_console(State *state);

void console_update(Window_Info *window, Events_Info *events, Time_Info *t);

void console_draw(Window_Info *window);

void console_free();

/**
 * Prints message to the console.
 */
void console_log(char *format, ...);

/**
 * Prints error message to the console.
 */
void console_error(char *format, ...);


@Introspect;
@RegisterCommand;
s64 add(s64 a, s64 b);

@Introspect;
@RegisterCommand;
void clear();


#endif
