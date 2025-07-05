#ifndef CONSOLE_H
#define CONSOLE_H

#include "game/plug.h"
#include "game/draw.h"
#include "core/graphics.h"
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
 * Adds contents of the buffer to the console.
 */
void console_add(char *buffer, s64 length);

/**
 * Prints formatted string to the console.
 */
void cprintf(char *format, ...);

/**
 * Executes console command specified by the string.
 */
void console_command(String str);

void init_console_commands();



#endif
