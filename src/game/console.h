#ifndef CONSOLE_H
#define CONSOLE_H

#include "game/plug.h"
#include "game/draw.h"
#include "game/graphics.h"
#include "core/structs.h"
#include "core/str.h"
#include "core/mathf.h"
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_keycode.h>



void init_console(Plug_State *state);

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

/**
 * This function parses console command given in a string form and immidiately executes it.
 */
void console_exec_command(String str);

void init_console_commands();



#endif
